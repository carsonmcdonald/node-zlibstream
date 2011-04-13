#ifndef NODE_ZLIBSTREAM_H_
#define NODE_ZLIBSTREAM_H_

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

#include <zlib.h>

using namespace node;
using namespace v8;

class ZLibStream : ObjectWrap 
{
  public:
    static Persistent<FunctionTemplate> s_ct;

    static void Init(v8::Handle<v8::Object> target);

    enum Flate { INFLATE, DEFLATE };
 
  protected:
    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    inline static v8::Handle<v8::Value> SetInflateDictionary(const v8::Arguments& args) { return SetInternalDictionary(args, INFLATE); }
    inline static v8::Handle<v8::Value> SetDeflateDictionary(const v8::Arguments& args) { return SetInternalDictionary(args, DEFLATE); }
    inline static v8::Handle<v8::Value> Inflate(const v8::Arguments& args) { return InternalFlate(args, INFLATE); }
    inline static v8::Handle<v8::Value> Deflate(const v8::Arguments& args) { return InternalFlate(args, DEFLATE); }
    inline static v8::Handle<v8::Value> ResetInflate(const v8::Arguments& args) { return ResetInternal(args, INFLATE); }
    inline static v8::Handle<v8::Value> ResetDeflate(const v8::Arguments& args) { return ResetInternal(args, DEFLATE); }

    ZLibStream() : ObjectWrap() 
    {
      inflate_stream.zalloc = Z_NULL; inflate_stream.zfree = Z_NULL; inflate_stream.opaque = Z_NULL;
      inflateInit(&inflate_stream);

      deflate_stream.zalloc = Z_NULL; deflate_stream.zfree = Z_NULL; deflate_stream.opaque = Z_NULL;
      deflateInit(&deflate_stream, Z_DEFAULT_COMPRESSION);

      inflate_dictionary_set = false;
    }

    ~ZLibStream()
    {
      inflateEnd(&inflate_stream);
      deflateEnd(&deflate_stream);
    }

  private:
    z_stream inflate_stream;
    z_stream deflate_stream;

    Persistent<Object> inflate_dictionary;
    bool inflate_dictionary_set;

    static v8::Handle<v8::Value> SetInternalDictionary(const v8::Arguments& args, enum Flate);
    static v8::Handle<v8::Value> ResetInternal(const v8::Arguments& args, enum Flate);
    static v8::Handle<v8::Value> InternalFlate(const v8::Arguments& args, enum Flate);
};

#endif // NODE_ZLIBSTREAM_H_
