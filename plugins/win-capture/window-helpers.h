#pragma once

#include <util/dstr.h>

enum window_priority {
	WINDOW_PRIORITY_CLASS,
	WINDOW_PRIORITY_TITLE,
	WINDOW_PRIORITY_EXE,
};

enum window_search_mode {
	INCLUDE_MINIMIZED,
	EXCLUDE_MINIMIZED
};

extern bool get_window_exe(struct dstr *name, HWND window);

extern void fill_window_list(obs_property_t *p, enum window_search_mode mode);

extern void build_window_strings(const char *str,
		char **class_name,
		char **title,
		char **exe);

extern void __declspec(dllexport) get_window_encode_strings(struct dstr *encode, const HWND window);

extern HWND find_window(enum window_search_mode mode,
		enum window_priority priority,
		const char *class_name,
		const char *title,
		const char *exe);
