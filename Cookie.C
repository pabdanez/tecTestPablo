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

namespace {

std::string HTTPONLY_PREFIX = "#HttpOnly_";
std::string UNKNOWN_TAG = "unknown";
std::string PARAMETER_SEPARATOR = ";";

std::string DOMAIN_TAG = "Domain";
std::string EXPIRES_TAG = "Expires";
std::string HTTPONLY_TAG = "HttpOnly";
std::string MAX_AGE_TAG = "Max-Age";
std::string PATH_TAG = "Path";
std::string SECURE_TAG = "Secure";
std::string PARTITIONED_TAG = "Partitioned";
std::string SAMESITE_TAG = "SameSite";

std::string STRICT_TAG = "Strict";
std::string LAX_TAG = "Lax";
std::string NONE_TAG = "None";

void TrimSpaces(std::string& Str)
{
   Str.erase(0, Str.find_first_not_of(' '));
}

void TrimQuotes(std::string& Str)
{
   if ((Str.length() > 1) && Str.front() == '"' && Str.back() == '"')
   {
      Str.pop_back();
      Str.erase(0);
   }
}

//this function doesn't handle UTF-8, but it works okay for the purposes of this example code
bool StrCaseEq(const std::string& Str1, const std::string& Str2)
{
#ifdef _WIN32
   return ((Str1.length() == Str2.length())
           && _stricmp(Str1.c_str(), Str2.c_str()) == 0);
#else
   return ((Str1.length() == Str2.length())        // Assume difference in a single operation, efficient
           && strcasecmp(Str1.c_str(), Str2.c_str()) == 0);
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
** bool NextParameter(const std::string& Str, const std::string& Separator,
**      size_t& Idx, std::string& Parameter)
**
** DESCRIPTION : Look for the next parameter in Str, starting at Idx
**
** RETURN VALUE: True if a parameter was found, otherwise false
**                                                                           */
/*=***************************************************************************/
bool NextParameter(const std::string& Str, const std::string& Separator, size_t& Idx, std::string& Parameter)
{
   if (Idx >= Str.length())
      return false;

   size_t PosSeparator = Str.find(Separator, Idx);
   Parameter = Str.substr(Idx, PosSeparator - Idx);
   Idx = PosSeparator;
   if (Idx != std::string::npos)
   {
      Idx++;
   }

   return true;
}

/*=****************************************************************************
**
** void SplitNameValue(const std::string& Parameter, std::string& Name,
      std::string& Value)
**
** DESCRIPTION : Split the incoming parameter into name and value
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void SplitNameValue(const std::string& Parameter, std::string& Name, std::string& Value)
{
   size_t EqPos= Parameter.find("=");
   Name = Parameter.substr(0, EqPos);

   if (EqPos == std::string::npos) // Equal not found
   {
      Value.clear();
   }
   else
   {
      Value = Parameter.substr(EqPos + 1);
      TrimQuotes(Value);             // Value field may have quotes
   }

   TrimSpaces(Name);
   TrimQuotes(Name);                // According to documentation, Name cannot have quotes
}

}  // Anonymous namespace


class CookieC                       // Could be splitted in .hpp and .cpp
{
 public:
   static CookieC *Create(const std::string& Name,
                          const std::string& Value,
                          const std::string& Domain,
                          const std::string& Path,
                          const std::string& Secure,
                          const std::string& Partitioned,
                          time_t      Expires,
                          const std::string& SameSite= "");

   CookieC();

   const std::string& GetName() const;    // They could be inline
   const std::string& GetValue() const;
   const std::string& GetDomain() const;
   const std::string& GetPath() const;
   const std::string& GetExpires() const;
   const std::string& GetSameSite() const;
   bool        IsStoredInPartitiondStorage() const;
   bool        IsSecure() const;
   bool        IsHttpOnly() const;
   bool        IsSessionCookie() const;

   bool        FromString(const std::string& Str, const std::string& Domain = "");  // Is it supposed to be possible to recall this method?
   const std::string& ToString() const;

 private:
   bool Init(const std::string& Name,
             const std::string& Value,
             const std::string& Domain,
             const std::string& Path,
             const std::string& Secure,
             const std::string& Partitioned,
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
   void SetPartitioned(const std::string& Partitioned);
   void SetPartitioned(bool Partitioned);
   void SetHttpOnly(bool HttpOnly);
   void SetSameSite(const std::string& SameSite);

   std::string mName, mValue, mDomain, mPath, mExpires;
   mutable std::string mHeaderFormat;
   bool          mSecure, mHttpOnly, mPartitioned;
   std::string mSameSite;
   // Max-Age not included as it is handled with Expires field
};

/*=****************************************************************************
**
** CookieC *CookieC::Create(const std::string& Name,
**    const std::string& Value,
**    const std::string& Domain,
**    const std::string& Path,
**    const std::string& Secure,
**    const std::string& Partitioned,
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
                         const std::string& Partitioned,
                         time_t      Expires,
                         const std::string& SameSite)
{
   CookieC *C = new CookieC();

   if (C)
   {
      if (!C->Init(Name, Value, Domain, Path, Secure, Partitioned, Expires, SameSite)) // Never happens as Init always returns true
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
   mPartitioned(),
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
                   const std::string& Partitioned,
                   time_t      Expires,
                   const std::string& SameSite)
{
   SetName(Name);
   SetValue(Value);
   SetDomain(Domain);
   SetPath(Path);
   SetExpires(Expires);
   SetSecure(Secure);
   SetPartitioned(Partitioned);
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
   if (Domain.starts_with(HTTPONLY_PREFIX))
   {
      mDomain = Domain.substr(HTTPONLY_PREFIX.length());
      mHttpOnly = true;
   }
   else
   {
      mDomain = Domain;
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
   if (Path != UNKNOWN_TAG)
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
   // Careful, if SetSecure() is evaluated after SetPartitioned()
   // there may be a case where Partitioned is true and Secure is false
   // That is wrong according to the documentation.
   // Impossible scenario in current code, but relevant for future developments.
   mSecure = StrCaseEq(Secure, SECURE_TAG);     // Originally checking TRUE -> bug?
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
   return mSecure;      // Probably better mSecure || mPartitioned but only mSecure should define this getter
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
** void CookieC::SetPartitioned(const std::string& Partitioned)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetPartitioned(const std::string& Partitioned)
{
   mPartitioned = StrCaseEq(Partitioned, PARTITIONED_TAG);
   SetSecure(true);              // Partitioned always implies Secure
}

void CookieC::SetPartitioned(bool Partitioned)
{
   mPartitioned = Partitioned;
   SetSecure(true);              // Partitioned always implies Secure
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
bool CookieC::IsStoredInPartitiondStorage() const
{
   return mPartitioned;
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
   // Maybe good idea to check if the result is one of the expected "Strict", "Lax", or "None"
   // As the description of the method specifies what values is it geting, it is "safe" not to check.
   mSameSite = SameSite;
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
   bool        IsNameSet  = false;
   bool        MaxAgeSet  = false;
   std::string Parameter, Name, Value;
   size_t         idx = 0;

   if (!Domain.empty())
      SetDomain(Domain);

   // As ostringstream is used, we may use istringstream here
   while(NextParameter(CookieStr, PARAMETER_SEPARATOR, idx, Parameter))
   {
      SplitNameValue(Parameter, Name, Value);

      if (Name.empty())
         continue;

      if (!IsNameSet)
      {
         SetName(Name);
         SetValue(Value);
         IsNameSet = true;
         continue;
      }

      switch (std::toupper(static_cast<unsigned char>(Name.front()))) // Necessary cast according to cppreference
      {
         case 'D':
            if (StrCaseEq(Name, DOMAIN_TAG))
               SetDomain(Value);
            break;
         case 'E':
            if (StrCaseEq(Name, EXPIRES_TAG))
               if (!MaxAgeSet)
                  SetExpires(Value);
            break;
         case 'H':
            if (StrCaseEq(Name, HTTPONLY_TAG))
               SetHttpOnly(true);
            break;
         case 'M':
            if (StrCaseEq(Name, MAX_AGE_TAG))
            {
               MaxAgeSet = true;
               int ValueMaxAge = stoi(Value);
               if (ValueMaxAge < 0)
                  ValueMaxAge = 0;
               /* Max-Age is # seconds from now. So expiration will be <Now> + Value */
               SetExpires(time(nullptr) + ValueMaxAge);
            }
            break;
         case 'P':
            if (StrCaseEq(Name, PATH_TAG))
               SetPath(Value);
            else if (StrCaseEq(Name, PARTITIONED_TAG))
            {
               SetPartitioned(true);
            }
            break;
         case 'S':
            if (StrCaseEq(Name, SECURE_TAG))
               SetSecure(true);
            else if (StrCaseEq(Name, SAMESITE_TAG))
            {
               SetSameSite(Value);
               if (StrCaseEq(Value, NONE_TAG))
                  SetSecure(true);
            }
            break;
      }
   }

   mHeaderFormat.clear();     // If FromString is called again, reset mHeaderFormat

   return IsNameSet;
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
         oss << "; " << EXPIRES_TAG << "=" << mExpires;

      if (!mDomain.empty())
         oss << "; " << DOMAIN_TAG<< "=" << mDomain;

      if (!mPath.empty())
         oss << "; " << PATH_TAG << "=" << mPath;

      if (!mSameSite.empty())
         oss << "; " << SAMESITE_TAG << "=" << mSameSite;

      if (mSecure)
         oss << "; " << SECURE_TAG;

      if (mPartitioned)
         oss << "; " << PARTITIONED_TAG;

      if (mHttpOnly)
         oss << "; " << HTTPONLY_TAG;

      mHeaderFormat = oss.str();
   }

   return mHeaderFormat;
}




int main(int argc, char* argv[])
{
    // Example usage
    std::string CookieStr = "name=value; domain=#HttpOnly_example.com; path=/; expires=Wed, 21 Oct 2023 07:28:00 GMT; Partitioned";
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
        std::cout << "Cookie Partitioned: " << Cookie->IsStoredInPartitiondStorage() << std::endl;


        delete Cookie;  // Could use unique_ptr to avoid explicit deletion
    }

    CookieC* Cookie2 = CookieC::Create("name", "value", "example.com", "/", "secure", "partitioned", time(nullptr), "Lax");
    if (Cookie2)
    {
      std::cout << "Cookie2: " << Cookie2->ToString() << std::endl;
       delete Cookie2;
    }

    std::cout << std::endl;
    return 0;
}
