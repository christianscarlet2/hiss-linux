#ifndef HISS_POKERCHAT_HPP
#define HISS_POKERCHAT_HPP
#include "mfc_compat.h"
extern CString _the_chat_message;   // defined in headless_stubs.cpp
inline void ComputeFirstPossibleNextChatTime() {}
inline double NextChatTime() { return 0; }
#endif
