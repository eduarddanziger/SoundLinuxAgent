// ReSharper disable CppClangTidyBugproneMacroParentheses
#pragma once

#ifndef DISALLOW_COPY
#define DISALLOW_COPY(TypeName) \
  TypeName(const TypeName&) = delete; \
  TypeName& operator=(const TypeName&) = delete
#endif //DISALLOW_COPY

#ifndef DISALLOW_MOVE
#define DISALLOW_MOVE(TypeName) \
  TypeName(TypeName&&) = delete; \
  TypeName& operator=(TypeName&&) = delete
#endif // DISALLOW_MOVE

#ifndef DISALLOW_COPY_MOVE
#define DISALLOW_COPY_MOVE(TypeName) \
  DISALLOW_COPY(TypeName); \
  DISALLOW_MOVE(TypeName)
#endif // DISALLOW_COPY_MOVE

#ifndef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete; \
  DISALLOW_COPY_MOVE(TypeName)
#endif // DISALLOW_IMPLICIT_CONSTRUCTORS

#ifndef AS_INTERFACE
#define AS_INTERFACE(TypeName) \
public: \
  virtual ~ TypeName () = default; \
protected: \
	TypeName () = default
#endif // AS_INTERFACE
