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

static void Free(void *ptr)
{
   if (ptr)
      free(ptr);
}

static char *Strdup(const char *OldString)
{
   char          *Ptr;

   if (!OldString)
      return nullptr;

   Ptr = (char *) malloc(strlen(OldString) + 1);
   if (!Ptr)
      return nullptr;

   strcpy(Ptr, OldString);
   return Ptr;
}

bool IsEmptyString(const char *Str)
{
   return (!Str || *Str == '\0');
}

char *TrimSpaces(char *Str)
{
   while (*Str == ' ')
      Str++;
   return Str;
}

//this function doesn't handle UTF-8, but it works okay for the purposes of this example code
bool StrCaseEq(const char *Str1, const char *Str2)
{
#ifdef _WIN32
   return (_stricmp(Str1, Str2) == 0);
#else
   return (strcasecmp(Str1, Str2) == 0);
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

class CookieC
{
 public:
   static CookieC *Create(const char *Name,
                          const char *Value,
                          const char *Domain,
                          const char *Path,
                          const char *Secure,
                          time_t      Expires,
                          const char *SameSite = nullptr);

   CookieC();
   CookieC(const CookieC &);
   CookieC &operator=(const CookieC &);
   ~CookieC();

   const char *GetName() const;
   const char *GetValue() const;
   const char *GetDomain() const;
   const char *GetPath() const;
   const char *GetExpires() const;
   const char *GetSameSite() const;
   bool        IsSecure() const;
   bool        IsHttpOnly() const;
   bool        IsSessionCookie() const;

   bool        FromString(const char *Str, const char *Domain = nullptr);
   const char *ToString() const;

 private:
   bool Init(const char *Name,
             const char *Value,
             const char *Domain,
             const char *Path,
             const char *Secure,
             time_t      Expires,
             const char *SameSite);

   void Assign(const CookieC &rhs);
   void Free();

   void SetName(const char *Name);
   void SetValue(const char *Value);
   void SetDomain(const char *Domain);
   void SetPath(const char *Path);
   void SetExpires(time_t Expires);
   void SetExpires(const char *Expires);
   void SetSecure(const char *Secure);
   void SetSecure(bool Secure);
   void SetHttpOnly(bool HttpOnly);
   void SetSameSite(const char *SameSite);

   char         *mName, *mValue, *mDomain, *mPath, *mExpires;
   mutable char *mHeaderFormat;
   bool          mSecure, mHttpOnly;
   char         *mSameSite;
};

/*=****************************************************************************
**
** int SplitStringIntoItems(const char *Str, char ***ItemListPtr, const char
**    *SepStr)
**
** DESCRIPTION : split <SepStr> separated string into items
**    
**    Returned itemlist and each item  must be Free(F)'d by caller,
**    use FreeArgCV(F)
**
** RETURN VALUE: no of items, items returned in ItemListPtr
**    REMEMBER TO Free(F) or FreeArgCV(F)
**                                                                           */
/*=***************************************************************************/
int SplitStringIntoItems(const char *Str, char ***ItemListPtr, const char *SepStr)
{
   char **ItemList = NULL;
   char  *TmpStr;
   char  *ItemPtr;
   int    NoOfItems;
   int    MaxNoOfItems;
   char  *Nxt;

   NoOfItems = 0;

   if (!IsEmptyString(Str))
   {
      TmpStr = Strdup(Str);
      if (TmpStr)
      {
         MaxNoOfItems = 20;
         ItemList     = (char **) malloc(MaxNoOfItems * sizeof(char *));
         if (!ItemList)
         {
            *ItemListPtr = NULL;
            Free(TmpStr);
            return -1;
         }

         ItemPtr = strtok_r(TmpStr, SepStr, &Nxt);
         while (ItemPtr != NULL)
         {
            if (NoOfItems == MaxNoOfItems)
            {
               ItemList = (char **) realloc(ItemList, (MaxNoOfItems + 20) * sizeof(char *));
               if (!ItemList)
               {
                  *ItemListPtr = NULL;
                  Free(TmpStr);
                  return -1;
               }
               MaxNoOfItems += 20;
            }

            ItemList[NoOfItems] = Strdup(ItemPtr);
            NoOfItems++;

            ItemPtr = strtok_r(NULL, SepStr, &Nxt);
         }

         Free(TmpStr);
      }
   }

   *ItemListPtr = ItemList;

   return NoOfItems;
}

/*=****************************************************************************
**
** CookieC *CookieC::Create(const char *Name,
**    const char *Value,
**    const char *Domain,
**    const char *Path,
**    const char *Secure,
**    time_t Expires,
**    const char *SameSite)
**
** DESCRIPTION: This class holds a cURL cookie.
**    
**    Cookies are obtained by the cURL call                                   \
**    curl_easy_getinfo(CURLINFO_COOKIELIST)
**                                                                           */
/*=***************************************************************************/
CookieC *CookieC::Create(const char *Name,
                         const char *Value,
                         const char *Domain,
                         const char *Path,
                         const char *Secure,
                         time_t      Expires,
                         const char *SameSite)
{
   CookieC *C = NULL;

   C = new CookieC();
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
   mName(nullptr),
   mValue(nullptr),
   mDomain(nullptr),
   mPath(nullptr),
   mExpires(nullptr),
   mHeaderFormat(nullptr),
   mSecure(false),
   mHttpOnly(false),
   mSameSite(nullptr)
{
}

/*=****************************************************************************
**
** bool CookieC::Init(const char *Name,
**    const char *Value,
**    const char *Domain,
**    const char *Path,
**    const char *Secure,
**    time_t Expires,
**    const char *SameSite)
**
** DESCRIPTION : Initialize object
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
bool CookieC::Init(const char *Name,
                   const char *Value,
                   const char *Domain,
                   const char *Path,
                   const char *Secure,
                   time_t      Expires,
                   const char *SameSite)
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
** void CookieC::Assign(const CookieC &rhs)
**
** DESCRIPTION : Assign values from one Cookie to another
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::Assign(const CookieC &rhs)
{
   mName         = Strdup(rhs.mName);
   mValue        = Strdup(rhs.mValue);
   mDomain       = Strdup(rhs.mDomain);
   mPath         = Strdup(rhs.mPath);
   mExpires      = Strdup(rhs.mExpires);
   mHeaderFormat = Strdup(rhs.mHeaderFormat);
   mSecure       = rhs.mSecure;
   mHttpOnly     = rhs.mHttpOnly;
   mSameSite     = Strdup(rhs.mSameSite);
}

/*=****************************************************************************
**
** void CookieC::Free()
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::Free()
{
   ::Free(mName);
   ::Free(mValue);
   ::Free(mDomain);
   ::Free(mPath);
   ::Free(mExpires);
   ::Free(mHeaderFormat);
   ::Free(mSameSite);
}

/*=****************************************************************************
**
** CookieC::CookieC(const CookieC &rhs)
**
** DESCRIPTION : Copy Constructor
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
CookieC::CookieC(const CookieC &rhs)
{
   Assign(rhs);
}

/*=****************************************************************************
**
** CookieC &CookieC::operator=(const CookieC &rhs)
**
** DESCRIPTION : Assignment operator
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
CookieC &CookieC::operator=(const CookieC &rhs)
{
   if (this != &rhs)
   {
      Free();
      Assign(rhs);
   }
   return *this;
}

/*=****************************************************************************
**
** CookieC::~CookieC()
**
** DESCRIPTION : Destructor
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
CookieC::~CookieC()
{
   Free();
}

/*=****************************************************************************
**
** void CookieC::SetName(const char *Name)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetName(const char *Name)
{
   mName = Strdup(Name);
}

/*=****************************************************************************
**
** const char *CookieC::GetName() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const char *CookieC::GetName() const
{
   return mName;
}

/*=****************************************************************************
**
** void CookieC::SetValue(const char *Value)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetValue(const char *Value)
{
   mValue = Strdup(Value);
}

/*=****************************************************************************
**
** const char *CookieC::GetValue() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const char *CookieC::GetValue() const
{
   return mValue;
}

/*=****************************************************************************
**
** void CookieC::SetDomain(const char *Domain)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetDomain(const char *Domain)
{
   if (strstr(Domain, "#HttpOnly_"))
   {
      mDomain   = Strdup(Domain + 10);
      mHttpOnly = true;
   }
   else
   {
      mDomain = Strdup(Domain);
   }
}

/*=****************************************************************************
**
** const char *CookieC::GetDomain() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const char *CookieC::GetDomain() const
{
   return mDomain;
}

/*=****************************************************************************
**
** void CookieC::SetPath(const char *Path)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetPath(const char *Path)
{
   if (strcmp(Path, "unknown") != 0)
      mPath = Strdup(Path);
}

/*=****************************************************************************
**
** const char *CookieC::GetPath() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const char *CookieC::GetPath() const
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

      mExpires = Strdup(TmpExpires);
   }
}

/*=****************************************************************************
**
** void CookieC::SetExpires(const char *Expires)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetExpires(const char *Expires)
{
   if (mExpires)
      free(mExpires);
   mExpires = Strdup(Expires);
}

/*=****************************************************************************
**
** const char *CookieC::GetExpires() const
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
const char *CookieC::GetExpires() const
{
   return mExpires;
}

/*=****************************************************************************
**
** void CookieC::SetSecure(const char *Secure)
**
** DESCRIPTION :
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetSecure(const char *Secure)
{
   mSecure = (StrCaseEq(Secure, "TRUE")) ? true : false;
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
   return IsEmptyString(mExpires);
}

/*=****************************************************************************
**
** void CookieC::SetSameSite(const char *SameSite)
**
** DESCRIPTION :
**
** PARAMETERS  : One of the values "Strict", "Lax", or "None"
**
** RETURN VALUE:
**                                                                           */
/*=***************************************************************************/
void CookieC::SetSameSite(const char *SameSite)
{
   mSameSite = Strdup(SameSite);
}

/*=****************************************************************************
**
** const char *CookieC::GetSameSite() const
**
** DESCRIPTION :
**
** RETURN VALUE: One of the values "Strict", "Lax", or "None"
**                                                                           */
/*=***************************************************************************/
const char *CookieC::GetSameSite() const
{
   return mSameSite;
}

/*=****************************************************************************
**
** bool CookieC::FromString(const char *CookieStr, const char *DomainUrl)
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
bool CookieC::FromString(const char *CookieStr, const char *Domain)
{
   bool        IsNameSet  = false;
   char      **Parameters = nullptr;
   int         Count;
   int         i;
   char       *Name;
   char       *Delim;
   const char *Value;

   if (Domain)
      SetDomain(Domain);

   Count = SplitStringIntoItems(CookieStr, &Parameters, ";");
   for (i = 0; i < Count; i++)
   {
      Name  = TrimSpaces(Parameters[i]);
      Delim = strchr(Name, '=');
      if (Delim)
      {
         *Delim = 0; /* Terminate Name                            */
         Value  = Delim + 1;
      }
      else
         Value = "";
      if (*Name == 0)
         continue;

      if (!IsNameSet)
      {
         /* First parameter must be Name/Value */
         if (Name[0] == '"' && Name[strlen(Name) - 1] == '"')
         {
            /* Name may be surrounded by double quotes */
            Name[strlen(Name) - 1] = 0;
            Name++;
         }
         SetName(Name);
         SetValue(Value);
         IsNameSet = true;
         continue;
      }

      switch (toupper(*Name))
      {
         case 'D':
            if (StrCaseEq(Name, "Domain"))
               SetDomain(Value);
            break;
         case 'E':
            if (StrCaseEq(Name, "Expires"))
               SetExpires(Value);
            break;
         case 'H':
            if (StrCaseEq(Name, "HttpOnly"))
               SetHttpOnly(true);
            break;
         case 'M':
            if (StrCaseEq(Name, "Max-Age"))
            {
               if (atoi(Value) > 0)
               {
                  /* Max-Age is # seconds from now. So expiration will be <Now> +
                     Value                                                      */
                  SetExpires(time(nullptr) + atoi(Value));
               }
            }
            break;
         case 'P':
            if (StrCaseEq(Name, "Path"))
               SetPath(Value);
            break;
         case 'S':
            if (StrCaseEq(Name, "Secure"))
               SetSecure(true);
            else if (StrCaseEq(Name, "SameSite"))
            {
               SetSameSite(Value);
               if (StrCaseEq(Value, "None"))
                  SetSecure(true);
            }
            break;
      }
   }

   ::Free(Parameters);
   return IsNameSet;
}

/*=****************************************************************************
**
** const char *CookieC::ToString() const
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
const char *CookieC::ToString() const
{
   if (!mHeaderFormat)
   {
      std::ostringstream oss;

      oss << mName << "=" << mValue;

      if (mExpires)
         oss << "; expires=" << mExpires;

      if (mDomain)
         oss << "; domain=" << mDomain;

      if (mPath)
         oss << "; path=" << mPath;

      if (mSecure)
         oss << "; secure";

      if (mHttpOnly)
         oss << "; httponly";

      mHeaderFormat = Strdup(oss.str().c_str());
   }

   return mHeaderFormat;
}




int main(int argc, char* argv[])
{
    // Example usage
    const char* CookieStr = "name=value; domain=example.com; path=/; expires=Wed, 21 Oct 2023 07:28:00 GMT; secure; httponly";
    CookieC* Cookie = new CookieC();
    if (Cookie)
    {
        Cookie->FromString(CookieStr);
        printf("Cookie Name: %s\n", Cookie->GetName());
        printf("Cookie Value: %s\n", Cookie->GetValue());
        printf("Cookie Domain: %s\n", Cookie->GetDomain());
        printf("Cookie Path: %s\n", Cookie->GetPath());
        printf("Cookie Expires: %s\n", Cookie->GetExpires());
        printf("Cookie Secure: %d\n", Cookie->IsSecure());
        printf("Cookie HttpOnly: %d\n", Cookie->IsHttpOnly());
        printf("Cookie SameSite: %s\n", Cookie->GetSameSite() ? Cookie->GetSameSite() : "(NULL)");

        delete Cookie;
    }

    CookieC* Cookie2 = CookieC::Create("name", "value", "example.com", "/", "secure", time(nullptr), "Lax");
    if (Cookie2)
    {
       printf("%s\n", Cookie2->ToString());
       delete Cookie2;
    }

    return 0;
}
