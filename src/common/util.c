#include <integrity/common/common.h>

/* START */
/* https://stackoverflow.com/a/46530956 */
float dc_safe_atof(const char* str) {
  unsigned char abc;
  float ret = 0, fac = 1;
  for (abc = 9; abc & 1; str++) {
    abc = *str == '-'                                   ? ((abc & 6) ? (abc & 14) : ((abc & 47) | 36))
          : *str == '+'                                 ? ((abc & 6) ? (abc & 14) : ((abc & 15) | 4))
          : *str > 47 && *str < 58                      ? abc | 18
          : (abc & 8) && *str == '.'                    ? (abc & 39) | 2
          : !(abc & 2) && (*str == ' ' || *str == '\t') ? (abc & 47) | 1
                                                        : abc & 46;
    if (abc & 16) {
      ret = (abc & 8) ? (*str - 48 + ret * 10) : ((*str - 48) / (fac *= 10) + ret);
    }
  }
  return (abc & 32) ? -ret : ret;
}

/* END */
