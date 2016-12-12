/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include <orthanc/OrthancCPlugin.h>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <json/value.h>

#if HAS_ORTHANC_EXCEPTION == 1
#  include <OrthancException.h>
#endif


namespace OrthancPlugins
{
  typedef void (*RestCallback) (OrthancPluginRestOutput* output,
                                const char* url,
                                const OrthancPluginHttpRequest* request);


  class PluginException
  {
  private:
    OrthancPluginErrorCode  code_;

  public:
    PluginException(OrthancPluginErrorCode code) : code_(code)
    {
    }

    OrthancPluginErrorCode GetErrorCode() const
    {
      return code_;
    }

    const char* GetErrorDescription(OrthancPluginContext* context) const;
  };


  class MemoryBuffer : public boost::noncopyable
  {
  private:
    OrthancPluginContext*      context_;
    OrthancPluginMemoryBuffer  buffer_;

  public:
    MemoryBuffer(OrthancPluginContext* context);

    ~MemoryBuffer()
    {
      Clear();
    }

    OrthancPluginMemoryBuffer* operator*()
    {
      return &buffer_;
    }

    // This transfers ownership
    void Assign(OrthancPluginMemoryBuffer& other);

    const char* GetData() const
    {
      if (buffer_.size > 0)
      {
        return reinterpret_cast<const char*>(buffer_.data);
      }
      else
      {
        return NULL;
      }
    }

    size_t GetSize() const
    {
      return buffer_.size;
    }

    void Clear();

    void ToString(std::string& target) const;

    void ToJson(Json::Value& target) const;

    bool RestApiGet(const std::string& uri,
                    bool applyPlugins);

    bool RestApiPost(const std::string& uri,
                     const char* body,
                     size_t bodySize,
                     bool applyPlugins);

    bool RestApiPut(const std::string& uri,
                    const char* body,
                    size_t bodySize,
                    bool applyPlugins);

    bool RestApiPost(const std::string& uri,
                     const std::string& body,
                     bool applyPlugins)
    {
      return RestApiPost(uri, body.empty() ? NULL : body.c_str(), body.size(), applyPlugins);
    }

    bool RestApiPut(const std::string& uri,
                    const std::string& body,
                    bool applyPlugins)
    {
      return RestApiPut(uri, body.empty() ? NULL : body.c_str(), body.size(), applyPlugins);
    }
  };


  class OrthancString : public boost::noncopyable
  {
  private:
    OrthancPluginContext*  context_;
    char*                  str_;

  public:
    OrthancString(OrthancPluginContext* context,
                  char* str);

    ~OrthancString()
    {
      Clear();
    }

    void Clear();

    const char* GetContent() const
    {
      return str_;
    }

    void ToString(std::string& target) const;

    void ToJson(Json::Value& target) const;
  };


  class OrthancConfiguration : public boost::noncopyable
  {
  private:
    OrthancPluginContext*  context_;
    Json::Value            configuration_;
    std::string            path_;

    std::string GetPath(const std::string& key) const;

  public:
    OrthancConfiguration() : context_(NULL)
    {
    }

    OrthancConfiguration(OrthancPluginContext* context);

    OrthancPluginContext* GetContext() const;

    const Json::Value& GetJson() const
    {
      return configuration_;
    }

    void GetSection(OrthancConfiguration& target,
                    const std::string& key) const;

    bool LookupStringValue(std::string& target,
                           const std::string& key) const;
    
    bool LookupIntegerValue(int& target,
                            const std::string& key) const;

    bool LookupUnsignedIntegerValue(unsigned int& target,
                                    const std::string& key) const;

    bool LookupBooleanValue(bool& target,
                            const std::string& key) const;

    bool LookupFloatValue(float& target,
                          const std::string& key) const;

    std::string GetStringValue(const std::string& key,
                               const std::string& defaultValue) const;

    int GetIntegerValue(const std::string& key,
                        int defaultValue) const;

    unsigned int GetUnsignedIntegerValue(const std::string& key,
                                         unsigned int defaultValue) const;

    bool GetBooleanValue(const std::string& key,
                         bool defaultValue) const;

    float GetFloatValue(const std::string& key,
                        float defaultValue) const;
  };

  class OrthancImage
  {
  private:
    OrthancPluginContext*  context_;
    OrthancPluginImage*    image_;

    void Clear();

    void CheckImageAvailable();

  public:
    OrthancImage(OrthancPluginContext*  context);

    OrthancImage(OrthancPluginContext*  context,
                 OrthancPluginImage*    image);

    OrthancImage(OrthancPluginContext*     context,
                 OrthancPluginPixelFormat  format,
                 uint32_t                  width,
                 uint32_t                  height);

    ~OrthancImage()
    {
      Clear();
    }

    void UncompressPngImage(const void* data,
                            size_t size);

    void UncompressJpegImage(const void* data,
                             size_t size);

    void DecodeDicomImage(const void* data,
                          size_t size,
                          unsigned int frame);

    OrthancPluginPixelFormat GetPixelFormat();

    unsigned int GetWidth();

    unsigned int GetHeight();

    unsigned int GetPitch();
    
    const void* GetBuffer();

    void CompressPngImage(MemoryBuffer& target);

    void CompressJpegImage(MemoryBuffer& target,
                           uint8_t quality);

    void AnswerPngImage(OrthancPluginRestOutput* output);

    void AnswerJpegImage(OrthancPluginRestOutput* output,
                         uint8_t quality);
  };


  bool RestApiGetJson(Json::Value& result,
                      OrthancPluginContext* context,
                      const std::string& uri,
                      bool applyPlugins);

  bool RestApiPostJson(Json::Value& result,
                       OrthancPluginContext* context,
                       const std::string& uri,
                       const char* body,
                       size_t bodySize,
                       bool applyPlugins);

  bool RestApiPutJson(Json::Value& result,
                      OrthancPluginContext* context,
                      const std::string& uri,
                      const char* body,
                      size_t bodySize,
                      bool applyPlugins);

  inline bool RestApiPostJson(Json::Value& result,
                              OrthancPluginContext* context,
                              const std::string& uri,
                              const std::string& body,
                              bool applyPlugins)
  {
    return RestApiPostJson(result, context, uri, body.empty() ? NULL : body.c_str(), 
                           body.size(), applyPlugins);
  }

  bool RestApiDelete(OrthancPluginContext* context,
                     const std::string& uri,
                     bool applyPlugins);

  inline bool RestApiPutJson(Json::Value& result,
                             OrthancPluginContext* context,
                             const std::string& uri,
                             const std::string& body,
                             bool applyPlugins)
  {
    return RestApiPutJson(result, context, uri, body.empty() ? NULL : body.c_str(), 
                          body.size(), applyPlugins);
  }

  bool RestApiDelete(OrthancPluginContext* context,
                     const std::string& uri,
                     bool applyPlugins);


  namespace Internals
  {
    template <RestCallback Callback>
    OrthancPluginErrorCode Protect(OrthancPluginRestOutput* output,
                                   const char* url,
                                   const OrthancPluginHttpRequest* request)
    {
      try
      {
        Callback(output, url, request);
        return OrthancPluginErrorCode_Success;
      }
      catch (OrthancPlugins::PluginException& e)
      {
        return e.GetErrorCode();
      }
#if HAS_ORTHANC_EXCEPTION == 1
      catch (Orthanc::OrthancException& e)
      {
        return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
      }
#endif
      catch (boost::bad_lexical_cast& e)
      {
        return OrthancPluginErrorCode_BadFileFormat;
      }
      catch (...)
      {
        return OrthancPluginErrorCode_Plugin;
      }
    }
  }

  
  template <RestCallback Callback>
  void RegisterRestCallback(OrthancPluginContext* context,
                            const std::string& uri,
                            bool isThreadSafe)
  {
    if (isThreadSafe)
    {
      OrthancPluginRegisterRestCallbackNoLock(context, uri.c_str(), Internals::Protect<Callback>);
    }
    else
    {
      OrthancPluginRegisterRestCallback(context, uri.c_str(), Internals::Protect<Callback>);
    }
  }
}
