#ifndef __TP_H__
#define __TP_H__

#ifdef __cplusplus
extern "C" {
#endif

struct tp_point {
	int x;
	int y;
};

int tp_suspend(void);
int tp_resume(void);
int tp_test_id(void);
int tp_test_point(struct tp_point *point);

#ifdef __cplusplus
}
#endif

#endif
