#ifndef jsc_context_hpp
#define jsc_context_hpp

#include <type_traits>
#include <vector>

#include "object.hpp"
#include "property.hpp"
#include "string.hpp"
#include "value.hpp"

// Testing
#include <iostream>

namespace jsc {

struct context_group {
  friend struct context;

  inline context_group() : _ref{JSContextGroupCreate()} {}
  inline ~context_group() { JSContextGroupRelease(_ref); }

  inline context_group(const context_group& group) : _ref{group._ref} {
    JSContextGroupRetain(_ref);
  }
  inline context_group& operator=(const context_group& group) {
    _ref = group._ref;
    JSContextGroupRetain(_ref);
    return *this;
  }

 private:
  JSContextGroupRef _ref;
};

struct context {
  friend struct value;
  friend struct object;

  inline context()
      : _ref{JSGlobalContextCreate(nullptr)}, _exception{undefined()} {}
  inline context(const context_group& group)
      : _ref{JSGlobalContextCreateInGroup(group._ref, nullptr)},
        _exception{undefined()} {}
  inline context(JSGlobalContextRef ref) : _ref{ref}, _exception{undefined()} {
    JSGlobalContextRetain(_ref);
  }
  inline ~context() { JSGlobalContextRelease(_ref); }

  inline context(const context& ctx) : _ref{ctx._ref}, _exception{undefined()} {
    JSGlobalContextRetain(_ref);
  }
  inline context& operator=(const context& ctx) {
    _ref = ctx._ref;
    JSGlobalContextRetain(_ref);
    return *this;
  }

  [[nodiscard]] inline value val(jsc::tag_undefined) {
    return {*this, JSValueMakeUndefined(_ref)};
  }
  [[nodiscard]] inline value val(jsc::tag_null) {
    return {*this, JSValueMakeNull(_ref)};
  }
  [[nodiscard]] inline value val(bool b) {
    return {*this, JSValueMakeBoolean(_ref, b)};
  }
  [[nodiscard]] inline value val(int i) { return val(static_cast<double>(i)); }
  [[nodiscard]] inline value val(double num) {
    return {*this, JSValueMakeNumber(_ref, num)};
  }
  [[nodiscard]] inline value val(const char* str) {
    const auto js_string = details::string_wrapper{str};
    return {*this, JSValueMakeString(_ref, js_string.managed_ref())};
  }
  [[nodiscard]] inline value val(const std::string& str) {
    const auto js_string = details::string_wrapper{str};
    return {*this, JSValueMakeString(_ref, js_string.managed_ref())};
  }
  [[nodiscard]] inline value val(object obj) { return obj; }

  [[nodiscard]] inline value undefined() { return val(jsc::undefined); }
  [[nodiscard]] inline value null() { return val(jsc::null); }

  [[nodiscard]] inline object root() {
    return {*this, JSContextGetGlobalObject(_ref)};
  }
  [[nodiscard]] inline object obj() {
    return {*this, JSObjectMake(_ref, nullptr, nullptr)};
  }

 private:
  using internal_callback_type =
      std::function<JSValueRef(JSContextRef, JSObjectRef, JSObjectRef, size_t,
                               const JSValueRef[], JSValueRef*)>;

  static JSValueRef callback_class_call(JSContextRef ctx, JSObjectRef function,
                                        JSObjectRef this_object,
                                        size_t argument_count,
                                        const JSValueRef arguments[],
                                        JSValueRef* exception);

  template <typename object_type>
  static void container_finalize(JSObjectRef container) {
    delete static_cast<object_type*>(JSObjectGetPrivate(container));
  }

  inline static JSClassRef callback_class() {
    static JSClassRef _callback_class = nullptr;
    if (_callback_class == nullptr) {
      JSClassDefinition def{kJSClassDefinitionEmpty};
      def.className = nullptr;
      def.attributes = kJSClassAttributeNone;
      def.callAsFunction = callback_class_call;
      def.finalize = container_finalize<internal_callback_type>;
      _callback_class = JSClassCreate(&def);
    }
    return _callback_class;
  }

  // Non-callable
  template <typename object_type>
  inline static JSClassRef container_class() {
    static JSClassRef _container_class = nullptr;
    if (_container_class == nullptr) {
      JSClassDefinition def{kJSClassDefinitionEmpty};
      def.className = nullptr;
      def.attributes = kJSClassAttributeNone;
      def.finalize = container_finalize<object_type>;
      _container_class = JSClassCreate(&def);
    }
    return _container_class;
  }

 public:
  template <typename object_type, typename... arg_type>
  inline object container(arg_type&&... args) {
    return {*this,
            JSObjectMake(_ref, container_class<object_type>(),
                         new object_type{std::forward<arg_type>(args)...})};
  }

  template <typename callback_type>
  inline object callable(callback_type callback) {
    auto callback_func = [callback{std::move(callback)}](
                             JSContextRef ctx, JSObjectRef,
                             JSObjectRef this_object, size_t argument_count,
                             const JSValueRef arguments[],
                             JSValueRef* exception) {
      context global_context{JSContextGetGlobalContext(ctx)};
      object this_obj{global_context, this_object};
      std::vector<value> vector;
      for (size_t i = 0; i < argument_count; i++) {
        vector.emplace_back(global_context, arguments[i]);
      }

      value result = global_context.undefined();
      value exception_placeholder = global_context.undefined();
      if constexpr (std::is_same_v<decltype(callback(global_context, this_obj,
                                                     std::move(vector),
                                                     &exception_placeholder)),
                                   void>) {
        callback(global_context, this_obj, std::move(vector),
                 &exception_placeholder);
      } else if constexpr (std::is_same_v<decltype(
                                              callback(global_context, this_obj,
                                                       std::move(vector),
                                                       &exception_placeholder)),
                                          value>) {
        result = callback(global_context, this_obj, std::move(vector),
                          &exception_placeholder);
      } else {
        result = global_context.val(callback(global_context, this_obj,
                                             std::move(vector),
                                             &exception_placeholder));
      }
      if (!exception_placeholder.is_undefined()) {
        *exception = exception_placeholder.ref();
      }
      return result.ref();
    };
    return {*this,
            JSObjectMake(_ref, callback_class(),
                         new internal_callback_type{std::move(callback_func)})};
  }

  [[nodiscard]] inline bool ok() const { return _exception.is_undefined(); }
  [[nodiscard]] inline const value& get_exception() const { return _exception; }
  inline void clear_exception() { _exception = undefined(); }

  value eval_script(const std::string& script,
                    const std::string& source_url = "<anonymous>");

 private:
  JSGlobalContextRef _ref;
  value _exception;

  inline void set_exception(value exception) {
    if (!exception.is_undefined()) {
      _exception = exception;
    }
  }

  template <typename throwable>
  auto try_throwable(throwable t) {
    JSValueRef exception = JSValueMakeUndefined(_ref);
    if constexpr (std::is_same_v<decltype(t(&exception)), void>) {
      t(&exception);
      set_exception({*this, exception});
    } else {
      auto result = t(&exception);
      set_exception({*this, exception});
      return std::move(result);
    }
  }

  inline void protect(JSValueRef val) const {
    JSGlobalContextRetain(_ref);
    JSValueProtect(_ref, val);
  }
  inline void unprotect(JSValueRef val) const {
    JSValueUnprotect(_ref, val);
    JSGlobalContextRelease(_ref);
  }

 public:
  [[nodiscard]] inline object error(const std::string& message) {
    // TODO: Add error subclassing
    const auto message_val = val(message);
    const auto message_ref = message_val.ref();
    return {*this,
            try_throwable([this, error_class{root()["Error"].get().to_object()},
                           &message_ref](auto exception) {
              return JSObjectCallAsConstructor(_ref, error_class.ref(), 1,
                                               &message_ref, exception);
            })};
  }
};

}  // namespace jsc

#endif  // jsc_context_hpp
