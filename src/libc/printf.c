#include <stdarg.h>
#include "os.h"

static int printf_d(int d, int comp_flag, char comp_char, int strict_len, int left_flag) {
  int ret = 0, base = 1;
  if (d < 0) {
	_putc('-');
	ret++;
	d = 0 - d;
  }
  else if (d == 0)
	ret++;
  while ((d / base) != 0) {
	base *= 10;
	ret++;
  }
  if (base != 1)
	base /= 10;
  if (comp_flag == 1 && left_flag == 1 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  while (base != 0) {
	_putc('0' +(d / base));
	d %= base;
	base /= 10;
  }
  if (comp_flag == 1 && left_flag == 0 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  if (strict_len > ret) return strict_len;
  return ret;
}

static int printf_x(int x, int comp_flag, char comp_char, int strict_len, int left_flag) {
  int ret = 0, base = 1;
  if (x < 0) {
	_putc('-');
	ret++;
	x = 0 - x;
  }
  else if (x == 0)
	ret++;
  while ((x / base) != 0) {
	base *= 16;
	ret++;
  }
  if (base != 1)
    base /= 16;
  if (comp_flag == 1 && left_flag == 1 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  while (base != 0) {
	if (x / base >= 0 && x / base <= 9)
	  _putc('0' + (x / base));
	else
	  _putc('a' + (x / base - 10));
	x %= base;
	base /= 16;
  }
  if (comp_flag == 1 && left_flag == 0 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  if (strict_len > ret) return strict_len;
  return ret;
}

static int printf_c(char c, int comp_flag, char comp_char, int strict_len, int left_flag) {
  int ret = 1;
  if (comp_flag == 1 && left_flag == 1 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  _putc(c);
  if (comp_flag == 1 && left_flag == 0 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  if (strict_len > ret) return strict_len;
  return ret;
}

static int printf_s(char * s, int comp_flag, char comp_char, int strict_len, int left_flag) {
  int ret = 0;
  if (comp_flag == 1 && left_flag == 1 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  while (*s) {
    _putc(*s);
	s++;
	ret++;
  }
  if (comp_flag == 1 && left_flag == 0 && strict_len > ret) {
	for (int i = 0; i < strict_len - ret; i++)
	  _putc(comp_char);
  }
  if (strict_len > ret) return strict_len;
  return ret;
}

int printf(const char *fmt, ...) {
  int ret = 0;
  va_list ap;
  va_start(ap, fmt);
  while (*fmt) {
	if (*fmt != '%') {
	  _putc(*fmt);
	  fmt++;
	  ret++;
	}
	else {
	  int sub_len = 0, comp_flag = 0, strict_len = 0, left_flag = 1, len_strict_arr_p = 0, base = 1;
	  char comp_char = ' ';
	  char len_strict_arr[8];
	  fmt++; // now begin with the char after '%'
	  if (*fmt == '-') {
		left_flag = 0;
		fmt++;
	  }
	  if (*fmt == '0') {
		comp_char = '0';
		fmt++;
	  }
	  while (*fmt >= '0' && *fmt <= '9') {
		if (comp_flag == 0) comp_flag = 1;
		len_strict_arr[len_strict_arr_p++] = *fmt;
		fmt++;
	  }
	  for (int i = len_strict_arr_p - 1; i >= 0; i--) {
		strict_len += base * (len_strict_arr[i] - '0');
		base *= 10;
	  }
	  switch (*fmt) {
	  case 'd': {
		int d_val;
		d_val = va_arg(ap, int);
		sub_len = printf_d(d_val, comp_flag, comp_char, strict_len, left_flag);
		fmt++;
		break;
	  }
	  case 'c': {
		char c_val;
		c_val = va_arg(ap, int);
		sub_len = printf_c(c_val, comp_flag, comp_char, strict_len, left_flag);
		fmt++;
		break;
	  }
	  case 's': {
		char * s_val;
		s_val = va_arg(ap, char*);
		sub_len = printf_s(s_val, comp_flag, comp_char, strict_len, left_flag);
		fmt++;
		break;
	  }
	  case 'x': {
		int x_val;
		x_val = va_arg(ap, int);
		sub_len = printf_x(x_val, comp_flag, comp_char, strict_len, left_flag);
		fmt++;
		break;
	  }
	  default:
		_putc(*fmt);
		fmt++;
		sub_len = 2;
	  }
	  ret += sub_len;
	}
  }
  return ret;
}