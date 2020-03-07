/*
Copyright (c) 2020 TINN by Saverio Castellano. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#if V8_MAJOR_VERSION >= 7 && V8_MINOR_VERSION >= 9
	#define TO_LOCAL_CHECKED .ToLocalChecked()
	#define CONTEXT_ARG context,
#else 
	#define TO_LOCAL_CHECKED 
	#define CONTEXT_ARG 
#endif		
