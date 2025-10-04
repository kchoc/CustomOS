#ifndef CLEANUP_H
#define CLEANUP_H

/*
 * DEFINE_CLASS(name, type, exit_code, init_code, init_args...):
 *		Defines a class with a constructor and destructor for a type.
 *		@exit - code to run in destructor
 *		@init - code to run in constructor
 */
#define DEFINE_CLASS(_name, _type, _exit, _init, _init_args...)		\
typedef _type class_##_name##_t;									\
static inline void class_##_name##_destructor(_type *p)				\
{ _type _T = *p; _exit; }											\
static inline _type class_##_name##_constructor(_init_args)			\
{ _type t = _init; return t; }

#define CLASS(_name, var)											\
	class_##_name##_t var __cleanup(class_##_name##_destructor) =	\
		class_##_name##_constructor

#endif // CLEANUP_H
