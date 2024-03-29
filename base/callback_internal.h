// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions and classes that help the
// implementation, and management of the Callback objects.

#ifndef BASE_CALLBACK_INTERNAL_H_
#define BASE_CALLBACK_INTERNAL_H_

#include <stddef.h>
#include <memory>
#include <type_traits>
#include <vector>

#include "base/atomic_ref_count.h"
#include "base/base_export.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace base {
namespace internal {
template <CopyMode copy_mode>
class CallbackBase;

// BindStateBase is used to provide an opaque handle that the Callback
// class can use to represent a function object with bound arguments.  It
// behaves as an existential type that is used by a corresponding
// DoInvoke function to perform the function execution.  This allows
// us to shield the Callback class from the types of the bound argument via
// "type erasure."
// At the base level, the only task is to add reference counting data. Don't use
// RefCountedThreadSafe since it requires the destructor to be a virtual method.
// Creating a vtable for every BindState template instantiation results in a lot
// of bloat. Its only task is to call the destructor which can be done with a
// function pointer.
class BindStateBase {
 protected:
  explicit BindStateBase(void (*destructor)(BindStateBase*))
      : ref_count_(0), destructor_(destructor) {}
  ~BindStateBase() = default;

 private:
  friend class scoped_refptr<BindStateBase>;
  template <CopyMode copy_mode>
  friend class CallbackBase;

  void AddRef();
  void Release();

  AtomicRefCount ref_count_;

  // Pointer to a function that will properly destroy |this|.
  void (*destructor_)(BindStateBase*);

  DISALLOW_COPY_AND_ASSIGN(BindStateBase);
};

// Holds the Callback methods that don't require specialization to reduce
// template bloat.
// CallbackBase<MoveOnly> is a direct base class of MoveOnly callbacks, and
// CallbackBase<Copyable> uses CallbackBase<MoveOnly> for its implementation.
template <>
class BASE_EXPORT CallbackBase<CopyMode::MoveOnly> {
 public:
  CallbackBase(CallbackBase&& c);
  CallbackBase& operator=(CallbackBase&& c);

  // Returns true if Callback is null (doesn't refer to anything).
  bool is_null() const { return bind_state_.get() == NULL; }

  // Returns the Callback into an uninitialized state.
  void Reset();

 protected:
  // In C++, it is safe to cast function pointers to function pointers of
  // another type. It is not okay to use void*. We create a InvokeFuncStorage
  // that that can store our function pointer, and then cast it back to
  // the original type on usage.
  using InvokeFuncStorage = void(*)();

  // Returns true if this callback equals |other|. |other| may be null.
  bool EqualsInternal(const CallbackBase& other) const;

  // Allow initializing of |bind_state_| via the constructor to avoid default
  // initialization of the scoped_refptr.  We do not also initialize
  // |polymorphic_invoke_| here because doing a normal assignment in the
  // derived Callback templates makes for much nicer compiler errors.
  explicit CallbackBase(BindStateBase* bind_state);

  // Force the destructor to be instantiated inside this translation unit so
  // that our subclasses will not get inlined versions.  Avoids more template
  // bloat.
  ~CallbackBase();

  scoped_refptr<BindStateBase> bind_state_;
  InvokeFuncStorage polymorphic_invoke_ = nullptr;
};

// CallbackBase<Copyable> is a direct base class of Copyable Callbacks.
template <>
class BASE_EXPORT CallbackBase<CopyMode::Copyable>
    : public CallbackBase<CopyMode::MoveOnly> {
 public:
  CallbackBase(const CallbackBase& c);
  CallbackBase(CallbackBase&& c);
  CallbackBase& operator=(const CallbackBase& c);
  CallbackBase& operator=(CallbackBase&& c);
 protected:
  explicit CallbackBase(BindStateBase* bind_state)
      : CallbackBase<CopyMode::MoveOnly>(bind_state) {}
  ~CallbackBase() {}
};

extern template class CallbackBase<CopyMode::MoveOnly>;
extern template class CallbackBase<CopyMode::Copyable>;

// A helper template to determine if given type is non-const move-only-type,
// i.e. if a value of the given type should be passed via std::move() in a
// destructive way. Types are considered to be move-only if they have a
// sentinel MoveOnlyTypeForCPP03 member: a class typically gets this from using
// the DISALLOW_COPY_AND_ASSIGN_WITH_MOVE_FOR_BIND macro.
// It would be easy to generalize this trait to all move-only types... but this
// confuses template deduction in VS2013 with certain types such as
// std::unique_ptr.
// TODO(dcheng): Revisit this when Windows switches to VS2015 by default.

template <typename T> struct IsMoveOnlyType {
  // Types YesType and NoType are guaranteed such that sizeof(YesType) <
  // sizeof(NoType).
  using YesType = char;
  struct NoType { YesType dummy[2]; };

  template <typename U>
  static YesType Test(const typename U::MoveOnlyTypeForCPP03*);

  template <typename U>
  static NoType Test(...);

  static const bool value = sizeof((Test<T>(0))) == sizeof(YesType) &&
                            !std::is_const<T>::value;
};

// Specialization of IsMoveOnlyType so that std::unique_ptr is still considered
// move-only, even without the sentinel member.
template <typename T, typename D>
struct IsMoveOnlyType<std::unique_ptr<T, D>> : std::true_type {};

// Specialization of std::vector, so that it's considered move-only if the
// element type is move-only. Allocator is explicitly ignored when determining
// move-only status of the std::vector.
template <typename T, typename Allocator>
struct IsMoveOnlyType<std::vector<T, Allocator>> : IsMoveOnlyType<T> {};

template <typename>
struct CallbackParamTraitsForMoveOnlyType;

template <typename>
struct CallbackParamTraitsForNonMoveOnlyType;

// TODO(tzik): Use a default parameter once MSVS supports variadic templates
// with default values.
// http://connect.microsoft.com/VisualStudio/feedbackdetail/view/957801/compilation-error-with-variadic-templates
//
// This is a typetraits object that's used to take an argument type, and
// extract a suitable type for forwarding arguments.
template <typename T>
struct CallbackParamTraits
    : std::conditional<IsMoveOnlyType<T>::value,
         CallbackParamTraitsForMoveOnlyType<T>,
         CallbackParamTraitsForNonMoveOnlyType<T>>::type {
};

template <typename T>
struct CallbackParamTraitsForNonMoveOnlyType {
  using ForwardType = const T&;
};

// Note that for array types, we implicitly add a const in the conversion. This
// means that it is not possible to bind array arguments to functions that take
// a non-const pointer. Trying to specialize the template based on a "const
// T[n]" does not seem to match correctly, so we are stuck with this
// restriction.
template <typename T, size_t n>
struct CallbackParamTraitsForNonMoveOnlyType<T[n]> {
  using ForwardType = const T*;
};

// See comment for CallbackParamTraits<T[n]>.
template <typename T>
struct CallbackParamTraitsForNonMoveOnlyType<T[]> {
  using ForwardType = const T*;
};

// Parameter traits for movable-but-not-copyable scopers.
//
// Callback<>/Bind() understands movable-but-not-copyable semantics where
// the type cannot be copied but can still have its state destructively
// transferred (aka. moved) to another instance of the same type by calling a
// helper function.  When used with Bind(), this signifies transferal of the
// object's state to the target function.
//
// For these types, the ForwardType must not be a const reference, or a
// reference.  A const reference is inappropriate, and would break const
// correctness, because we are implementing a destructive move.  A non-const
// reference cannot be used with temporaries which means the result of a
// function or a cast would not be usable with Callback<> or Bind().
template <typename T>
struct CallbackParamTraitsForMoveOnlyType {
  using ForwardType = T;
};

// CallbackForward() is a very limited simulation of C++11's std::forward()
// used by the Callback/Bind system for a set of movable-but-not-copyable
// types.  It is needed because forwarding a movable-but-not-copyable
// argument to another function requires us to invoke the proper move
// operator to create a rvalue version of the type.  The supported types are
// whitelisted below as overloads of the CallbackForward() function. The
// default template compiles out to be a no-op.
//
// In C++11, std::forward would replace all uses of this function.  However, it
// is impossible to implement a general std::forward without C++11 due to a lack
// of rvalue references.
//
// In addition to Callback/Bind, this is used by PostTaskAndReplyWithResult to
// simulate std::forward() and forward the result of one Callback as a
// parameter to another callback. This is to support Callbacks that return
// the movable-but-not-copyable types whitelisted above.
template <typename T>
typename std::enable_if<!IsMoveOnlyType<T>::value, T>::type& CallbackForward(
    T& t) {
  return t;
}

template <typename T>
typename std::enable_if<IsMoveOnlyType<T>::value, T>::type CallbackForward(
    T& t) {
  return std::move(t);
}

}  // namespace internal
}  // namespace base

#endif  // BASE_CALLBACK_INTERNAL_H_
