#pragma once

// NetConstants.h - Shared Network Constants
//
// Shared between client and server to ensure buffer sizes stay in sync.

#define DEF_MSGBUFFERSIZE	60000
#define DEF_ITEMNAME		42

// Account field sizes (buffer = wire size + null terminator)
#define DEF_ACCOUNT_NAME    11   // 10 chars + null (wire size is 10)
#define DEF_ACCOUNT_PASS    11   // 10 chars + null (wire size is 10)
#define DEF_ACCOUNT_EMAIL   51   // 50 chars + null
