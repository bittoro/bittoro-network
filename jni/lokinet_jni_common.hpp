#ifndef LOKINET_JNI_COMMON_HPP
#define LOKINET_JNI_COMMON_HPP

#include <jni.h>
#include <util/string_view.hpp>
#include <functional>

/// visit string as native bytes
/// jvm uses some unholy encoding internally so we convert it to utf-8
template < typename T, typename V >
static T
VisitStringAsStringView(JNIEnv* env, jobject str, V visit)
{
  const jclass stringClass = env->GetObjectClass(str);
  const jmethodID getBytes =
      env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");

  const jstring charsetName = env->NewStringUTF("UTF-8");
  const jbyteArray stringJbytes =
      (jbyteArray)env->CallObjectMethod(str, getBytes, charsetName);
  env->DeleteLocalRef(charsetName);

  const size_t length = env->GetArrayLength(stringJbytes);
  jbyte* pBytes       = env->GetByteArrayElements(stringJbytes, NULL);

  T result = visit(llarp::string_view((const char*)pBytes, length));

  env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);
  env->DeleteLocalRef(stringJbytes);

  return std::move(result);
}

/// cast jni buffer to T *
template < typename T >
static T*
FromBuffer(JNIEnv* env, jobject o)
{
  if(o == nullptr)
    return nullptr;
  return static_cast< T* >(env->GetDirectBufferAddress(o));
}

/// get T * from object member called membername
template < typename T >
static T*
FromObjectMember(JNIEnv* env, jobject self, const char* membername)
{
  jclass cl      = env->GetObjectClass(self);
  jfieldID name  = env->GetFieldID(cl, membername, "Ljava/nio/Buffer;");
  jobject buffer = env->GetObjectField(self, name);
  return FromBuffer< T >(env, buffer);
}

/// visit object string member called membername as bytes
template < typename T, typename V >
static T
VisitObjectMemberStringAsStringView(JNIEnv* env, jobject self,
                                    const char* membername, V v)
{
  jclass cl     = env->GetObjectClass(self);
  jfieldID name = env->GetFieldID(cl, membername, "Ljava/lang/String;");
  jobject str   = env->GetObjectField(self, name);
  return VisitStringAsStringView< T, V >(env, str, v);
}

/// get object member int called membername
template < typename Int_t >
Int_t
GetObjectMemberAsInt(JNIEnv* env, jobject self, const char* membername)
{
  jclass cl     = env->GetObjectClass(self);
  jfieldID name = env->GetFieldID(cl, membername, "I");
  return env->GetIntField(self, name);
}

/// get implementation on jni type
template < typename T >
T*
GetImpl(JNIEnv* env, jobject self)
{
  return FromObjectMember< T >(env, self, "impl");
}

#endif