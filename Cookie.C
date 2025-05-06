/*
 ==========
 Assignment
 ==========

 The code below defines a class, CookieC, that represents an HTTP cookie.
 CookieC::FromString can be used to set the state from a cookie string.
 CookieC::ToString can be used to serialize the state in a cookie string.
 The cookie string format is described here:
 https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie

 A main function a the bottom of the file demonstrates the current usage
 of the class.

 The assignment is to change the code to use std::string instead of char*
 where this is relevant. The public member functions can also be changed.


 Compile
 =======
 Windows MSVC: cl /EHsc /Tp Cookie.C
 LLVM/GCC: g++ -std=c++20 -o Cookie Cookie.C

 */


#include <time.h>
#include <sstream>
#include <string>
#include <cstring>
#include <cassert>
#include <iostream>

char *TrimSpaces(char *Str) 
{
   while (*Str == ' ')
      Str++;
   return Str;
}

//this function doesn't handle UTF-8, but it works okay for the purposes of this example code
bool StrCaseEq(const std::string& Str1, const std::string& Str2)
{
#ifdef _WIN32
   return (_stricmp(Str1.c_str(), Str2.c_str()) == 0);
#else
   return (strcasecmp(Str1.c_str(), Str2.c_str()) == 0);
#endif
}

//align Windows with other platforms
#ifdef _WIN32
#define strtok_r strtok_s

struct tm *gmtime_r(const time_t *clock, struct tm *res)
{
   errno_t err = gmtime_s(res, clock);
   if (err)
      return nullptr;
   return res;
}
#endif

/*=****************************************************************************
**
** std::string findValue
**
** DESCRIPTION : Look for the specified field in a string and gets its value
**
** RETURN VALUE: String containing the value for that field
**                                                                           */
/*=***************************************************************************/
std::string findValue(const std::string& str, const std::string& field)
{
   // TODO: IMPROVE SEARCH -> UPPERCASES? QUOTES? WHITE SPACES?
   std::string foundValue = "";
   size_t posDomain = str.find(field);
   if(posDomain != std::string::npos)
   {
      size_t posStartDomain = posDomain + field.size();
      size_t posEndDomain = str.find(";", posDomain);
      size_t lenDomain = posEndDomain - posStartDomain;
      foundValue = str.substr(posStartDomain, lenDomain);
   }

   return foundValue;
}

class CookieC
{
 public:
   static CookieC *Create(const std::string& Name,
                          const std::string& Value,
                          const std::string& Domain,
                          const std::string& Path,
                          const std::string& Secure,
                          time_t      Expires,
                          const std::string& SameSite= "");

   CookieC();

   const std::string& GetName() const;
   const std::string& GetValue() const;
   const std::string& GetDomain() const;
   const std::string& GetPath() const;
   const std::string& GetExpires() const;
   const std::string& GetSameSite() const;
   bool        IsSecure() const;
   bool        IsHttpOnly() const;
   bool        IsSessionCookie() const;

   bool        FromString(const std::string& Str, const std::string& Domain = "");
   const std::string& ToString() const;

 private:
   bool Init(const std::string& Name,
             const std::string& Value,
             const std::string& Domain,
             const std::string& Path,
             const std::string& Secure,
             time_t      Expires,
             const std::string& SameSite);

   void SetName(const std::string& Name);
   void SetValue(const std::string& Value);
   void SetDomain(const std::string& Domain);
   void SetPath(const std::string& Path);
   void SetExpires(time_t Expires);
   void SetExpires(const std::string& Expires);
   void SetSecure(const std::string& Secure);
   void SetSecure(bool Secure);
   void SetHttpOnly(bool HttpOnly);
   void SetSameSite(const std::string& SameSite);

   std::string mName, mValue, mDomain, mPath, mExpires;
   mutable std::string mHeaderFormat;
   bool          mSecure, mHttpOnly;
   std::string mSameSite;
   // Max-Age and Partitioned not included?
};

/*=****************************************************************************
**
** CookieC *CookieC::Create(const std::string& Name,
**    const std::string& Value,
**    const std::string& Domain,
**    const std::string& Path,
**    const std::string& Secure,
**    time_t Expires,
**    const std::string& SameSite)
**
** DESCRIPTION: This class holds a cURL cookie.
**    
**    Cookies are obtained by the cURL call                                   \
**    curl_easy_getinfo(CURLINFO_COOKIELIST)
**                                                                           */
/*=***************************************************************************/
CookieC *CookieC::Create(const std::string& Name,
                         const std::string& Value,
                         const std::string& Domain,
                         const std::string& Path,
                         const std::string& Secure,
                         time_t      Expires,
                         const std::string& SameSite)
{
   CookieC *C = new CookieC();

   if (C)
   {
      if (!C->Init(Name, Value, Domain, Path, Secure, Expires, SameSite))
      {
         delete (C);
         return NULL;
      }
   }

   return C;
}

/*=****************************************************************************
**
** CookieC::CookieC()
**
** DESCRIPTION : Constructor
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
CookieC::CookieC() :
   mName(),
   mValue(),
   mDomain(),
   mPath(),
   mExpires(),
   mHeaderFormat(),
   mSecure(),
   mHttpOnly(),
   mSameSite()
{
}

/*=****************************************************************************
**
** bool CookieC::Init(const std::string& Name,
**    const std::string& Value,
**    const std::string& Domain,
**    const std::string& Path,
**    const std::string& Secure,
**    time_t Expires,
**    const std::string& SameSite)
**
** DESCRIPTION : Initialize object
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
bool CookieC::Init(const std::string& Name,
                   const std::string& Value,
                   const std::string& Domain,
                   const std::string& Path,
                   const std::string& Secure,
                   time_t      Expires,
                   const std::string& SameSite)
{
   SetName(Name);
   SetValue(Value);
   SetDomain(Domain);
   SetPath(Path);
   SetExpires(Expires);
   SetSecure(Secure);
   SetSameSite(SameSite);

   return true;
}

/*=****************************************************************************
**
** void CookieC::SetName(const std::string& Name)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetName(const std::string& Name)
{
   mName = Name;
}

/*=****************************************************************************
**
** const std::string& CookieC::GetName() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::GetName() const
{
   return mName;
}

/*=****************************************************************************
**
** void CookieC::SetValue(const std::string& Value)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetValue(const std::string&  Value)
{
   mValue = Value;
}

/*=****************************************************************************
**
** const std::string& CookieC::GetValue() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::GetValue() const
{
   return mValue;
}

/*=****************************************************************************
**
** void CookieC::SetDomain(const std::string& Domain)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetDomain(const std::string& Domain)
{
   size_t posSubStr = Domain.find("#HttpOnly_");   // TODO: This is wrong, assumes that the subStr can be in the middle of the string
   if(posSubStr == std::string::npos)
   {
      mDomain = Domain;
   }
   else
   {
      mDomain = Domain.substr(posSubStr + 10); // 10 = len of "#HttpOnly_" // TODO: Set constant?
      mHttpOnly = true;
   }
}

/*=****************************************************************************
**
** const std::string& CookieC::GetDomain() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::GetDomain() const
{
   return mDomain;
}

/*=****************************************************************************
**
** void CookieC::SetPath(const std::string& Path)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetPath(const std::string& Path)
{
   if (Path != "unknown") // TODO: Use constant
      mPath = Path;
   else
   mPath.clear(); // If the cookie uses FromString and the new value is unknown, the content would be outdated.
}

/*=****************************************************************************
**
** const std::string& CookieC::GetPath() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::GetPath() const
{
   return mPath;
}

/*=****************************************************************************
**
** void CookieC::SetExpires(time_t Expires)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetExpires(time_t Expires)
{
   const char DAYS[7][3 + 1] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
   const char MONS[12][3 + 1] =
      {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (Expires != 0) // persistent cookie
   {
      char      TmpExpires[29 + 1];
      size_t    Res;
      struct tm Tm;

      memset(&Tm, 0, sizeof(Tm));
      if (!gmtime_r(&Expires, &Tm))
         return;

      // Write expire in RFC1123 format, make room for %a and %b with x's
      Res = strftime(TmpExpires, sizeof(TmpExpires), "xxx, %d xxx %Y %H:%M:%S GMT", &Tm);
      assert(Res > 0);
      if (Res == 0)
         return;

      // write %a and %b locale independently in time string
      memcpy(TmpExpires, DAYS[Tm.tm_wday], 3);
      memcpy(TmpExpires + 3 + 1 + 1 + 2 + 1, MONS[Tm.tm_mon], 3);

      mExpires = TmpExpires;
   }
}

/*=****************************************************************************
**
** void CookieC::SetExpires(const std::string& Expires)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetExpires(const std::string& Expires)
{
   mExpires = Expires;
}

/*=****************************************************************************
**
** const std::string& CookieC::GetExpires() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::GetExpires() const
{
   return mExpires;
}

/*=****************************************************************************
**
** void CookieC::SetSecure(const std::string& Secure)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetSecure(const std::string& Secure)
{
   mSecure = StrCaseEq(Secure, "Secure"); // TODO: Add constant
}

void CookieC::SetSecure(bool Secure)
{
   mSecure = Secure;
}

/*=****************************************************************************
**
** bool CookieC::IsSecure() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
bool CookieC::IsSecure() const
{
   return mSecure;
}

/*=****************************************************************************
**
** void CookieC::SetHttpOnly(bool HttpOnly)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetHttpOnly(bool HttpOnly)
{
   mHttpOnly = HttpOnly;
}

/*=****************************************************************************
**
** bool CookieC::IsHttpOnly() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
bool CookieC::IsHttpOnly() const
{
   return mHttpOnly;
}

/*=****************************************************************************
**
** bool CookieC::IsSessionCookie() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
bool CookieC::IsSessionCookie() const
{
   return mExpires.empty();
}

/*=****************************************************************************
**
** void CookieC::SetSameSite(const std::string& SameSite)
**
** DESCRIPTION :
**
** PARAMETERS  : One of the values "Strict", "Lax", or "None"
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetSameSite(const std::string& SameSite)
{
   mSameSite = SameSite;   // TODO: Check if the result is one of the expected?
}

/*=****************************************************************************
**
** const std::string& CookieC::GetSameSite() const
**
** DESCRIPTION :
**
** RETURN VALUE: One of the values "Strict", "Lax", or "None"
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::GetSameSite() const
{
   return mSameSite;
}

/*=****************************************************************************
**
** bool CookieC::FromString(const std::string& CookieStr, const std::string& DomainUrl)
**
** DESCRIPTION : Parse header formatted string into cookie values (as         \
**    described                                                               \
**    https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie)
**    
**    <name>=<value>[; <name>=<value>]...
**    [; expires=<date>][; domain=<domain_name>]
**    [; path=<some_path>][; secure][; httponly]
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
bool CookieC::FromString(const std::string& CookieStr, const std::string& Domain)
{
   // TODO: Improve search process to handle uppercases, quotes, and whitespaces
   // TODO: Replicate mechanism to detect name and value
   
   // TODO: Consider Partitioned? Max-age?
   
   if (!Domain.empty())
      SetDomain(Domain);

   std::string foundValue = findValue(CookieStr, "name="); 
   if(foundValue != "")
   {
      SetName("name");
      SetValue(foundValue);
   }
   else
      return false;

   foundValue = findValue(CookieStr, "domain=");
   if(foundValue != "")
      SetDomain(foundValue);

   foundValue = findValue(CookieStr, "path=");
   if(foundValue != "")
      SetPath(foundValue);

   if(CookieStr.find("httponly") != std::string::npos)
      SetHttpOnly(true);

   if(CookieStr.find("secure") != std::string::npos)
      SetSecure(true);

   foundValue = findValue(CookieStr, "SameSite=");
   if(foundValue != "")
   {
      SetSameSite(foundValue);
      if(foundValue == "None")
         SetSecure(true);
   }

   foundValue = findValue(CookieStr, "expires=");
   if(foundValue != "")
      SetExpires(foundValue);

   mHeaderFormat.clear(); // In order to be reseted if FromString is used again anytime

   return true;
}

/*=****************************************************************************
**
** const std::string& CookieC::ToString() const
**
** DESCRIPTION : To header formatted string (as described                     \
**    https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie)
**
**    <name>=<value>[; <name>=<value>]...
**    [; expires=<date>][; domain=<domain_name>]
**    [; path=<some_path>][; secure][; httponly]
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const std::string& CookieC::ToString() const
{
   if (mHeaderFormat.empty())
   {
      std::ostringstream oss;

      oss << mName << "=" << mValue;

      if (!mExpires.empty())
         oss << "; Expires=" << mExpires;

      if (!mDomain.empty())
         oss << "; Domain=" << mDomain;

      if (!mPath.empty())
         oss << "; Path=" << mPath;

      if (!mSameSite.empty())
         oss << "; SameSite=" << mSameSite;

      if (mSecure)
         oss << "; Secure";

      if (mHttpOnly)
         oss << "; HttpOnly";

      mHeaderFormat = oss.str();
   }

   return mHeaderFormat;
}




int main(int argc, char* argv[])
{
    // Example usage
    std::string CookieStr = "name=value; domain=example.com; path=/; expires=Wed, 21 Oct 2023 07:28:00 GMT; secure; httponly";
    CookieC* Cookie = new CookieC();
    if (Cookie)
    {
        Cookie->FromString(CookieStr);
        std::cout << "Cookie Name: " << Cookie->GetName() << std::endl;
        std::cout << "Cookie Value: " << Cookie->GetValue() << std::endl;
        std::cout << "Cookie Domain: " << Cookie->GetDomain() << std::endl;
        std::cout << "Cookie Path: " << Cookie->GetPath() << std::endl;
        std::cout << "Cookie Expires: " << Cookie->GetExpires() << std::endl;
        std::cout << "Cookie Secure: " << Cookie->IsSecure() << std::endl;
        std::cout << "Cookie HttpOnly: " << Cookie->IsHttpOnly() << std::endl;
        std::cout << "Cookie SameSite: " << (Cookie->GetSameSite().empty() ? "(NULL)" : Cookie->GetSameSite()) << std::endl;


        delete Cookie;  // Could use unique_ptr to avoid explicit deletion
    }

    CookieC* Cookie2 = CookieC::Create("name", "value", "example.com", "/", "secure", time(nullptr), "Lax");
    if (Cookie2)
    {
      std::cout << "Cookie2: " << Cookie2->ToString() << std::endl;
       delete Cookie2;
    }

    std::cout << std::endl;
    return 0;
}
