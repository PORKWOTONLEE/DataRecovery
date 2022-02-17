#ifndef __CIVETWEB_H__
#define __CIVETWEB_H__

int Civetweb_Init(struct mg_context **pctx);
int Civetweb_Set_Handler(struct mg_context *ctx);
int Civetweb_Stop(struct mg_context *ctx);

#endif
