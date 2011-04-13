#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

#include <zlib.h>
#include <cstring>
#include <cstdlib>

#include "node-zlibstream.h"

using namespace v8;
using namespace node;

#define THROW_BAD_ARGS v8::ThrowException(v8::Exception::TypeError(v8::String::New("Bad arguments")))
#define THROW_ERROR(msg) v8::ThrowException(v8::Exception::TypeError(v8::String::New((msg == NULL ? "No know error message." : msg))))

Handle<Value> ZLibStream::New(const Arguments& args)
{
  if (!args.IsConstructCall()) 
  {
    return FromConstructorTemplate(s_ct, args);
  }

  HandleScope scope;

  ZLibStream* zlibstream = new ZLibStream();
  zlibstream->Wrap(args.This());

  return args.This();
}

Handle<Value> ZLibStream::SetInternalDictionary(const Arguments& args, enum ZLibStream::Flate flate)
{
  HandleScope scope;

  if (args.Length() < 1 || !Buffer::HasInstance(args[0]))
  {
    return THROW_BAD_ARGS;
  }

  ZLibStream* zlibstream = ObjectWrap::Unwrap<ZLibStream>(args.This());

  if(flate == INFLATE)
  {
    zlibstream->inflate_dictionary = Persistent<Object>::New(args[0]->ToObject());
    zlibstream->inflate_dictionary_set = true;
  }
  else
  {
    Bytef* dict = (Bytef*)Buffer::Data(args[0]->ToObject());
    int dictlen = Buffer::Length(args[0]->ToObject());

    if (deflateSetDictionary(&(zlibstream->deflate_stream), dict, dictlen) != Z_OK) 
    {
      return THROW_ERROR(zlibstream->deflate_stream.msg);
    } 
  }

  return Undefined();
}

Handle<Value> ZLibStream::InternalFlate(const Arguments& args, enum ZLibStream::Flate flate)
{
  HandleScope scope;

  if (args.Length() < 1 || !Buffer::HasInstance(args[0]))
  {
    return THROW_BAD_ARGS;
  }

  ZLibStream* zlibstream = ObjectWrap::Unwrap<ZLibStream>(args.This());

  int length = 0;
  Local<Object> input = args[0]->ToObject();
  if(flate == INFLATE)
  {
    zlibstream->inflate_stream.next_in = (Bytef*)Buffer::Data(input);
    length = zlibstream->inflate_stream.avail_in = Buffer::Length(input);
  }
  else
  {
    zlibstream->deflate_stream.next_in = (Bytef*)Buffer::Data(input);
    length = zlibstream->deflate_stream.avail_in = Buffer::Length(input);
  }

  void* result = NULL;
  int compressed = 0, available = 0;

  do 
  {
    result = realloc(result, compressed + (flate == INFLATE ? 2 : 1) * length);
    if (!result)
    {
      return THROW_ERROR("Could not allocate memory.");
    }

    int ret = -1;
    if(flate == INFLATE)
    {
      zlibstream->inflate_stream.avail_out = 2 * length;
      zlibstream->inflate_stream.next_out = (Bytef*)result + compressed;

      ret = inflate(&(zlibstream->inflate_stream), Z_SYNC_FLUSH);
      if (ret == Z_NEED_DICT) 
      {
        if(!zlibstream->inflate_dictionary_set)
        {
          free(result);
          return THROW_ERROR("A dictionary is required to inflate this stream but none was provided.");
        }

        Bytef* dict = (Bytef*)Buffer::Data(zlibstream->inflate_dictionary);
        int dictlen = Buffer::Length(zlibstream->inflate_dictionary);

        ret = inflateSetDictionary(&(zlibstream->inflate_stream), dict, dictlen);
        if (ret != Z_OK) 
        {
          free(result);
          return THROW_ERROR(zError(ret));
        }

        ret = inflate(&(zlibstream->inflate_stream), Z_SYNC_FLUSH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) 
        {
          free(result);
          return THROW_ERROR(zError(ret));
        }
      }

      compressed += (2 * length - zlibstream->inflate_stream.avail_out);
      available = zlibstream->inflate_stream.avail_out;
    }
    else
    {
      zlibstream->deflate_stream.avail_out = length;
      zlibstream->deflate_stream.next_out = (Bytef*)result + compressed;

      ret = deflate(&(zlibstream->deflate_stream), Z_SYNC_FLUSH);

      compressed += (length - zlibstream->deflate_stream.avail_out);
      available = zlibstream->deflate_stream.avail_out;
    }

    if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) 
    {
      free(result);
      return THROW_ERROR(zError(ret));
    }

  } while (available == 0);

  Buffer* output = Buffer::New((char *)result, compressed);
  free(result);

  return scope.Close(Local<Value>::New(output->handle_));
}

Handle<Value> ZLibStream::ResetInternal(const Arguments& args, enum ZLibStream::Flate flate)
{
  HandleScope scope;

  if (args.Length() != 0)
  {
    return THROW_BAD_ARGS;
  }

  ZLibStream* zlibstream = ObjectWrap::Unwrap<ZLibStream>(args.This());

  int rc = -1;
  if(flate == INFLATE)
  {
    rc = inflateReset(&(zlibstream->inflate_stream));

    if(!zlibstream->inflate_dictionary_set)
    {
      zlibstream->inflate_dictionary.Dispose();
      zlibstream->inflate_dictionary_set = false;
    }
  }
  else
  {
    rc = deflateReset(&(zlibstream->deflate_stream));
  }

  if (rc != Z_OK) 
  {
    return THROW_ERROR("Could not reset the stream.");
  }

  return Undefined();
}

void ZLibStream::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);

  s_ct = Persistent<FunctionTemplate>::New(t);
  s_ct->InstanceTemplate()->SetInternalFieldCount(1);
  s_ct->SetClassName(String::NewSymbol("ZLibStream"));

  NODE_SET_PROTOTYPE_METHOD(s_ct, "setInflateDictionary", SetInflateDictionary);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "setDeflateDictionary", SetDeflateDictionary);

  NODE_SET_PROTOTYPE_METHOD(s_ct, "inflate", Inflate);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "deflate", Deflate);
  
  NODE_SET_PROTOTYPE_METHOD(s_ct, "resetInflate", ResetInflate);
  NODE_SET_PROTOTYPE_METHOD(s_ct, "resetDeflate", ResetDeflate);

  target->Set(String::NewSymbol("ZLibStream"), s_ct->GetFunction());
}

Persistent<FunctionTemplate> ZLibStream::s_ct;

extern "C" 
{
  static void init (Handle<Object> target)
  {
    ZLibStream::Init(target);
  }

  NODE_MODULE(zlibstream_binding, init);
}
