#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
// 必须是 GET 或者 HEAD 方法,否则返回 405 Not Allowed
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }
// 丢弃请求中的包体
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK)
    {
        return rc;
    }
    /* 设置返回的 Content-Type。注意,ngx_str_t 有一个很方便的初始化宏 ngx_string,它可以把
    ngx_str_t 的 data 和 len 成员都设置好 */
    ngx_str_t type = ngx_string("text/plain");
// 返回的包体内容
    ngx_str_t response = ngx_string("Hello World!");
// 设置返回状态码
    r->headers_out.status = NGX_HTTP_OK;
// 响应包是有包体内容的,需要设置 Content-Length 长度
    r->headers_out.content_length_n = response.len;
// 设置 Content-Type
    r->headers_out.content_type = type;
// 发送 HTTP 头部
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        return rc;
    }
// 构造 ngx_buf_t 结构体准备发送包体
    ngx_buf_t *b;
    b = ngx_palloc(r->pool, sizeof(ngx_buf_t));
    u_char *filename = (u_char*)"/home/lje/software/nginx-1.4.2/objs/lje.txt";
    b->in_file = 1;
    b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
    b->file->fd = ngx_open_file(filename, NGX_FILE_RDONLY|NGX_FILE_NONBLOCK, NGX_FILE_OPEN, 0);
    b->file->log = r->connection->log;
    b->file->name.data = filename;
    b->file->name.len = sizeof("lje.txt") - 1;

    if (b->file->fd <= 0){
        return NGX_HTTP_NOT_FOUND;
    }

    if (ngx_file_info(filename, &b->file->info) == NGX_FILE_ERROR) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    r->headers_out.content_length_n = b->file->info.st_size;
    b->file_pos = 0;
    b->file_last = b->file->info.st_size;

    ngx_pool_cleanup_t* cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_file_t));
    if (cln == NULL){
        return NGX_ERROR;
    }
    cln->handler = ngx_pool_cleanup_file;
    ngx_pool_cleanup_file_t *clnf = cln->data;
    clnf->fd = b->file->fd;
    clnf->name = b->file->name.data;
    clnf->log = r->pool->log;
//    b = ngx_create_temp_buf(r->pool, response.len);
//    if (b == NULL)
//    {
//        return NGX_HTTP_INTERNAL_SERVER_ERROR;
//    }
//// 将 Hello World 复制到 ngx_buf_t 指向的内存中
//    ngx_memcpy(b->pos, response.data, response.len);
//// 注意,一定要设置好 last 指针
//    b->last = b->pos + response.len;
//// 声明这是最后一块缓冲区
//    b->last_buf = 1;
// 构造发送时的 ngx_chain_t 结构体
    ngx_chain_t out;
// 赋值 ngx_buf_t
    out.buf = b;
// 设置 next 为 NULL
    out.next = NULL;
    /* 最后一步为发送包体,发送结束后 HTTP 框架会调用 ngx_http_finalize_request 方法结束请求 */
    return ngx_http_output_filter(r, &out);
}

static char* ngx_http_mytest(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_http_core_loc_conf_t * clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_mytest_handler;
    return NGX_CONF_OK;

}

static ngx_command_t  ngx_http_mytest_commands[] =
{
    {
        ngx_string("mytest"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
        ngx_http_mytest,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};



static ngx_http_module_t  ngx_http_mytest_module_ctx =
{
    NULL,                          /* preconfiguration */
    NULL,           /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */
    NULL,       /* create location configuration */
    NULL /* merge location configuration */
};


ngx_module_t  ngx_http_mytest_module =
{
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx, /* module context */
    ngx_http_mytest_commands,   /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};
