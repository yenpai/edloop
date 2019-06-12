#ifndef _EDLIB_OBJECT_H_
#define _EDLIB_OBJECT_H_

typedef struct  edobject edobject;
typedef void (* edobject_finalize_cb) (edobject *);

struct edobject {
	unsigned int         refer_count;
	edobject_finalize_cb finalize_cb;
};

void       edobject_unref(edobject *);
edobject * edobject_ref(edobject *);
void       edobject_base_init(edobject *, edobject_finalize_cb);

#define _EDOBJ_EXTEND_METHOD_MACRO_(TYPE, NAME) \
	static inline edobject * NAME##_to_object(TYPE * o) { return (edobject *) o; } \
	static inline TYPE * NAME##_ref(TYPE * o) { return (TYPE *) edobject_ref((edobject *) o); } \
	static inline void NAME##_unref(TYPE * o) { edobject_unref((edobject *) o); }

#endif
