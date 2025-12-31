#pragma once

namespace Steinberg {
namespace Vst {

#ifdef DEVELOPMENT
static DECLARE_UID (AMegalayProcessorUID, 0x36EC68FE, 0x9ACf41E7, 0x851715FB, 0x00000000);
static DECLARE_UID (AMegalayControllerUID, 0xD0F765BB, 0x620C482C, 0x86CCEA5B, 0x00000000);
#else
static DECLARE_UID(AMegalayProcessorUID, 0x36EC68FE, 0x9ACf41E7, 0x851715FB, 0x2D552E37);
static DECLARE_UID(AMegalayControllerUID, 0xD0F765BB, 0x620C482C, 0x86CCEA5B, 0xF8d66833);
#endif

}} // namespaces

