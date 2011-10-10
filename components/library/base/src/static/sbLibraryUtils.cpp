/* vim: set sw=2 : */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#include "sbLibraryUtils.h"

#include "sbMediaListEnumArrayHelper.h"

#include <vector>

#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIProxyObjectManager.h>
#include <nsIThread.h>
#include <nsIURI.h>

#include <nsArrayUtils.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsStringAPI.h>
#include <nsThreadUtils.h>

#include <sbILibrary.h>
#include <sbILocalDatabaseSmartMediaList.h>
#include <sbIMediaList.h>
#include <sbIMediaItem.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>

#include <sbArrayUtils.h>
#include <sbFileUtils.h>
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include "sbMediaListEnumSingleItemHelper.h"
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

static nsresult
FindByProperties(sbIMediaList * aList,
                 sbIPropertyArray * aProperties,
                 nsIArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aProperties);

  nsresult rv;

  nsCOMPtr<sbIMediaListEnumerationListener> enumerator;
  nsRefPtr<sbMediaListEnumSingleItemHelper> single;
  if (aCopies) {
    enumerator = sbMediaListEnumArrayHelper::New(aCopies);
  }
  else {
    single = sbMediaListEnumSingleItemHelper::New();
    enumerator = do_QueryInterface(single);
  }
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  rv = aList->EnumerateItemsByProperties(
                                        aProperties,
                                        enumerator,
                                        sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  // If we're not getting all the copies, just see if we found one, if not
  // return not available error
  if (!aCopies) {
    nsCOMPtr<sbIMediaItem> item = single->GetItem();
    if (!item) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  return NS_OK;
}

static nsresult
FindByOrigin(sbIMediaList * aList,
             nsString const & aOriginLibGuid,
             nsString const & aOriginItemGuid,
             nsIArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aOriginLibGuid.IsEmpty()) {
    rv = properties->AppendProperty(
                                 NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                                 aOriginLibGuid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                  aOriginItemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  return FindByProperties(aList, properties, aCopies);
}

static nsresult
FindByContentURL(sbIMediaList * aList,
                 nsString const & aContentURL,
                 nsIArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                  aContentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  return FindByProperties(aList, properties, aCopies);
}

static nsresult
FindByOriginURL(sbIMediaList * aList,
                 nsString const & aOriginURL,
                 nsIArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                  aOriginURL);
  NS_ENSURE_SUCCESS(rv, rv);

  return FindByProperties(aList, properties, aCopies);
}

/* static */
nsresult sbLibraryUtils::GetItemInLibrary(/* in */  sbIMediaItem*  aMediaItem,
                                          /* in */  sbILibrary*    aLibrary,
                                          /* out */ sbIMediaItem** aItemCopy)
{
  /* this is an internal method, just assert/crash, don't be nice */
  NS_ASSERTION(aMediaItem, "no item to look up!");
  NS_ASSERTION(aLibrary, "no library to search in!");
  NS_ASSERTION(aItemCopy, "null return value pointer!");

  nsresult rv;

  nsCOMPtr<nsIMutableArray> theCopies =
    do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbLibraryUtils::FindCopiesByID(aMediaItem, aLibrary, theCopies);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;
  rv = theCopies->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  // Look for originals
  if (count == 0) {
    rv = sbLibraryUtils::FindOriginalsByID(aMediaItem, aLibrary, theCopies);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = theCopies->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (count != 0) {
    nsCOMPtr<sbIMediaItem> item = do_QueryElementAt(theCopies, 0, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    item.forget(aItemCopy);
    return NS_OK;
  }

  // give up
  *aItemCopy = nsnull;
  return NS_OK;
}

nsresult
sbLibraryUtils::FindItemsWithSameURL(sbIMediaItem * aMediaItem,
                                     sbIMediaList * aMediaList,
                                     nsIMutableArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aMediaList);

  nsresult rv;
  PRBool foundOne = PR_FALSE;
  nsString url;

  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                               url);
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (url.IsEmpty()) {
    rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                 url);
    if (rv != NS_ERROR_NOT_AVAILABLE) {
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (!url.IsEmpty()) {
    rv = FindByContentURL(aMediaList,
                          url,
                          aCopies);
    if (rv != NS_ERROR_NOT_AVAILABLE) {
      NS_ENSURE_SUCCESS(rv, rv);
      PRUint32 length;
      if (aCopies) {
        rv = aCopies->GetLength(&length);
        NS_ENSURE_SUCCESS(rv, rv);
        foundOne = length != 0;
      }
      else {
        foundOne = PR_TRUE;
      }
    }
    rv = FindByOriginURL(aMediaList,
                         url,
                         aCopies);
    if (rv != NS_ERROR_NOT_AVAILABLE) {
      NS_ENSURE_SUCCESS(rv, rv);
      PRUint32 length;
      if (aCopies) {
        rv = aCopies->GetLength(&length);
        NS_ENSURE_SUCCESS(rv, rv);
        foundOne |= length != 0;
      }
      else {
        foundOne = PR_TRUE;
      }
    }
  }
  return (aCopies || foundOne) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult sbLibraryUtils::FindCopiesByID(sbIMediaItem * aMediaItem,
                                        sbIMediaList * aList,
                                        nsIMutableArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;

  nsString guid;
  rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FindByOrigin(aList, nsString(), guid, aCopies);
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aCopies)
      return NS_OK;
  }

  nsString originLibID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                               originLibID);
  if (rv == NS_ERROR_NOT_AVAILABLE || originLibID.IsEmpty()) {
    if (!aCopies)
      return NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsString originItemID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originItemID);
  if (rv == NS_ERROR_NOT_AVAILABLE || originItemID.IsEmpty()) {
    if (!aCopies)
      return NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = properties->AppendProperty(
                               NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                               originLibID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                                  originItemID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FindByProperties(aList, properties, aCopies);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    if (!aCopies)
      return NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

 /**
  * Searches aList for items that have ID's that match aMediaItem's origin ID
  * \param aItem The item to find a match for
  * \param aLibrary The library to look in
  * \return The media items found, or null
  */
nsresult sbLibraryUtils::FindOriginalsByID(sbIMediaItem * aMediaItem,
                                           sbIMediaList * aList,
                                           nsIMutableArray * aCopies)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aList);

  nsresult rv;

  nsString originID;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                               originID);
  if (rv == NS_ERROR_NOT_AVAILABLE || originID.IsEmpty()) {
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aCopies)
      return NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
  }

  nsCOMPtr<nsIArray> copies;
  rv = aList->GetItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_GUID),
                                 originID,
                                 getter_AddRefs(copies));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;
  rv = copies->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aCopies) {
    rv = sbAppendnsIArray(copies, aCopies);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (count == 0) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetContentLength(/* in */  sbIMediaItem * aItem,
                                          /* out */ PRInt64      * _retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv = aItem->GetContentLength(_retval);

  if(NS_FAILED(rv) || !*_retval) {
    // try to get the length from disk
    nsCOMPtr<sbIMediaItem> item(aItem);

    if (!NS_IsMainThread()) {
      // Proxy item to get contentURI.
      // Note that we do *not* call do_GetProxyForObject if we're already on
      // the main thread - doing that causes us to process the next pending event
      nsCOMPtr<nsIThread> target;
      rv = NS_GetMainThread(getter_AddRefs(target));

      rv = do_GetProxyForObject(target,
                                NS_GET_IID(sbIMediaItem),
                                aItem,
                                NS_PROXY_SYNC,
                                getter_AddRefs(item));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIURI> contentURI;
    rv = item->GetContentSrc(getter_AddRefs(contentURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(contentURI, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    // note that this will abort if this is not a local file.  This is the
    // desired behaviour.

    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = file->GetFileSize(_retval);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                            sbAutoString(*_retval));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::SetContentLength(/* in */  sbIMediaItem * aItem,
                                          /* in */  nsIURI       * aURI)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(aURI);

  nsresult rv;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  // note that this will abort if this is not a local file.  This is the
  // desired behaviour.

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 length;
  rv = file->GetFileSize(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                          sbAutoString(length));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetOriginItem(/* in */ sbIMediaItem*   aItem,
                                       /* out */ sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // Get the origin library and item GUIDs.
  nsAutoString originLibraryGUID;
  nsAutoString originItemGUID;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                          originLibraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                          originItemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the origin item.
  nsCOMPtr<sbILibraryManager> libraryManager =
    do_GetService("@getnightingale.com/Nightingale/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbILibrary> library;
  rv = libraryManager->GetLibrary(originLibraryGUID, getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = library->GetItemByGuid(originItemGUID, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

inline
nsCOMPtr<nsIIOService> GetIOService(nsresult & rv)
{
  // Get the IO service.
  if (NS_IsMainThread()) {
    return do_GetIOService(&rv);
  }
  return do_ProxiedGetService(NS_IOSERVICE_CONTRACTID, &rv);
}

/* static */
nsresult sbLibraryUtils::GetContentURI(nsIURI*  aURI,
                                       nsIURI** _retval,
                                       nsIIOService * aIOService)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIURI> uri = aURI;

  // On Windows, convert "file:" URI's to lower-case.
#ifdef XP_WIN
  nsresult rv;
  PRBool isFileScheme;
  rv = uri->SchemeIs("file", &isFileScheme);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isFileScheme) {
    // Get the URI spec.
    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    // Convert the URI spec to lower case.
    ToLowerCase(spec);

    // Regenerate the URI.
    nsCOMPtr<nsIIOService> ioService = aIOService ? aIOService :
                                                    GetIOService(rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ioService->NewURI(spec, nsnull, nsnull, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif // XP_WIN

  // Return results.
  NS_ADDREF(*_retval = uri);

  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetMediaListContentType(sbIMediaList *aMediaList,
                                                 PRUint16 *aListContentType)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aListContentType);

  nsresult rv;

  *aListContentType = sbIMediaList::CONTENTTYPE_NONE;

  nsCOMPtr<sbILocalDatabaseSmartMediaList> smartList =
    do_QueryInterface(aMediaList, &rv);
  if (NS_FAILED(rv))
    rv = aMediaList->GetListContentType(aListContentType);
  else
    rv = smartList->GetListContentType(aListContentType);

  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
nsresult sbLibraryUtils::GetFileContentURI(nsIFile* aFile,
                                           nsIURI** _retval)
{
  NS_ENSURE_ARG_POINTER(aFile);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsIURI> uri;
  nsresult rv;

  // Get the file URI.
  rv = sbNewFileURI(aFile, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert URI to a content URI.
  return GetContentURI(uri, _retval);
}

/**
 * Enumerator class that populates the array it is given
 */
class MediaItemArrayCreator : public sbIMediaListEnumerationListener
{
public:
  MediaItemArrayCreator(nsCOMArray<sbIMediaItem> & aMediaItems) :
    mMediaItems(aMediaItems)
   {}
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER
private:
  nsCOMArray<sbIMediaItem> & mMediaItems;
};

NS_IMPL_ISUPPORTS1(MediaItemArrayCreator,
                   sbIMediaListEnumerationListener)

NS_IMETHODIMP MediaItemArrayCreator::OnEnumerationBegin(sbIMediaList*,
                                                         PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP MediaItemArrayCreator::OnEnumeratedItem(sbIMediaList*,
                                                      sbIMediaItem* aItem,
                                                      PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);

  PRBool const added = mMediaItems.AppendObject(aItem);
  NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP MediaItemArrayCreator::OnEnumerationEnd(sbIMediaList*,
                                                      nsresult)
{
  return NS_OK;
}


nsresult
sbLibraryUtils::GetItemsByProperty(sbIMediaList * aMediaList,
                                   nsAString const & aPropertyName,
                                   nsAString const & aValue,
                                   nsCOMArray<sbIMediaItem> & aMediaItems) {
  nsRefPtr<MediaItemArrayCreator> creator = new MediaItemArrayCreator(aMediaItems);
  return aMediaList->EnumerateItemsByProperty(aPropertyName,
                                              aValue,
                                              creator,
                                              sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
}

/**
 * Enumerator used to build a list of media lists for a content type
 */
class sbLUMediaListEnumerator : public sbIMediaListEnumerationListener
{
public:
  sbLUMediaListEnumerator(PRUint32 aContentType)
   : mContentType(aContentType)
   {}
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  nsresult GetMediaLists(nsIArray ** aMediaLists)
  {
    return CallQueryInterface(mMediaLists, aMediaLists);
  }
private:
  nsCOMPtr<nsIMutableArray> mMediaLists;
  PRUint32 mContentType;
};

NS_IMPL_ISUPPORTS1(sbLUMediaListEnumerator,
                   sbIMediaListEnumerationListener)

NS_IMETHODIMP sbLUMediaListEnumerator::OnEnumerationBegin(sbIMediaList*,
                                                          PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  mMediaLists = do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP sbLUMediaListEnumerator::OnEnumeratedItem(sbIMediaList*,
                                                        sbIMediaItem* aItem,
                                                        PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mMediaLists, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  nsCOMPtr<sbIMediaList> list = do_QueryInterface(aItem);
  if (list) {
    bool include = mContentType == sbIMediaList::CONTENTTYPE_MIX;
    if (!include) {
      PRUint16 contentType;
      rv = list->GetListContentType(&contentType);
      NS_ENSURE_SUCCESS(rv, rv);

      include = (contentType & mContentType) != 0;
    }
    if (include) {
      rv = mMediaLists->AppendElement(list, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }
  *_retval = sbIMediaListEnumerationListener::CONTINUE;

  return NS_OK;
}

NS_IMETHODIMP sbLUMediaListEnumerator::OnEnumerationEnd(sbIMediaList*,
                                                        nsresult)
{
  return NS_OK;
}

nsresult sbLibraryUtils::GetMediaListByContentType(sbILibrary * aLibrary,
                                                   PRUint32 aContentType,
                                                   nsIArray ** aMediaLists)
{
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_ARG_POINTER(aMediaLists);

  nsresult rv;
  nsString propIsList(NS_LITERAL_STRING(SB_PROPERTY_ISLIST));
  nsString propIsHidden(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN));
  nsString propTrue(NS_LITERAL_STRING("1"));
  nsString propFalse(NS_LITERAL_STRING("0"));
  PRUint16 enumType = sbIMediaList::ENUMERATIONTYPE_SNAPSHOT;

  nsRefPtr<sbLUMediaListEnumerator> enumerator =
    new sbLUMediaListEnumerator(aContentType);

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = properties->AppendProperty(propIsList, propTrue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = properties->AppendProperty(propIsHidden, propFalse);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aLibrary->EnumerateItemsByProperties(properties, enumerator, enumType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = enumerator->GetMediaLists(aMediaLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Returns the equality operator for the content property
 */
nsresult
sbLibraryUtils::GetEqualOperator(sbIPropertyOperator ** aOperator)
{
  nsresult rv;

  nsCOMPtr<sbIPropertyManager> manager =
    do_GetService("@getnightingale.com/Nightingale/Properties/PropertyManager;1",
                  &rv);
  nsCOMPtr<sbIPropertyInfo> info;
  rv = manager->GetPropertyInfo(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString opName;
  rv = info->GetOPERATOR_EQUALS(opName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the operator.
  rv = info->GetOperator(opName, aOperator);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Suggest a unique name for playlist.
 */
nsresult
sbLibraryUtils::SuggestUniqueNameForPlaylist(sbILibrary *aLibrary,
                                             nsAString const & aListName,
                                             nsAString & aName)
{
  nsresult rv;

  aName = aListName;
  nsCOMPtr<nsIArray> mediaLists;
  rv = aLibrary->GetItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                                    NS_LITERAL_STRING("1"),
                                    getter_AddRefs(mediaLists));
  if (rv != NS_ERROR_NOT_AVAILABLE) {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 listLength;
  rv = mediaLists->GetLength(&listLength);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 nameLength = aListName.Length();
  nsTArray<PRUint64> listIDs;
  nsString listName;
  PRUint64 availableId = 1;
  for (PRUint32 i = 0; i < listLength; ++i) {
    nsCOMPtr<sbIMediaList> mediaList = do_QueryElementAt(mediaLists, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mediaList->GetName(listName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!listName.IsEmpty()) {
      nsDependentSubstring subString(listName, 0, nameLength);
      if (subString.Equals(aListName)) {
        PRUint32 listNameLength = listName.Length();
        if (listNameLength == nameLength) {
          listIDs.AppendElement(1);
        }
        else {
          nsDependentSubstring idString(listName,
                                        nameLength + 1,
                                        listNameLength);
          PRUint64 id = nsString_ToUint64(idString, &rv);
          if (rv == NS_ERROR_INVALID_ARG)
            continue;
          listIDs.AppendElement(id);
        }
      }
    }
  }

  while (1) {
    if (!listIDs.Contains(availableId))
      break;

    ++availableId;
  }

  if (availableId > 1) {
    aName.Append(NS_LITERAL_STRING(" "));
    AppendInt(aName, availableId);
  }

  return NS_OK;
}

nsresult
sbLibraryUtils::LinkCopy(sbIMediaItem * aOriginal, sbIMediaItem * aCopy)
{
  NS_ENSURE_ARG_POINTER(aOriginal);
  NS_ENSURE_ARG_POINTER(aCopy);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> props =
    do_CreateInstance(
               "@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1",
               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString playlistGuid;
  rv = aOriginal->GetGuid(playlistGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = props->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINITEMGUID),
                             playlistGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> playlistLib;
  rv = aOriginal->GetLibrary(getter_AddRefs(playlistLib));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString playlistLibGuid;
  rv = playlistLib->GetGuid(playlistLibGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = props->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINLIBRARYGUID),
                             playlistLibGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aCopy->SetProperties(props);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

