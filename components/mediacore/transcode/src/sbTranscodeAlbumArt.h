/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
*/

#ifndef __SB_TRANSCODEALBUMART_H__
#define __SB_TRANSCODEALBUMART_H__

#include <nsIFile.h>

#include <imgIContainer.h>

#include <nsCOMPtr.h>
#include <nsStringAPI.h>

#include <sbITranscodeAlbumArt.h>
#include <sbIMediaItem.h>
#include <sbIDeviceCapabilities.h>

/* Helper class for transcoding album art and writing that art to the media
   file's metadata
 */
class sbTranscodeAlbumArt : public sbITranscodeAlbumArt
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODEALBUMART

  sbTranscodeAlbumArt();

  nsresult IsValidSizeForRange(sbIDevCapRange *aRange,
                               PRInt32 aVal,
                               PRBool *aIsValid);

  nsresult IsValidSizeForFormat(sbIImageFormatType *aFormat,
                                PRBool *aIsValid);

  nsresult GetTargetFormat(nsCString aMimeType,
                           PRInt32 *aWidth,
                           PRInt32 *aHeight);
protected:
  virtual ~sbTranscodeAlbumArt();

private:
  nsCOMPtr<nsIArray>      mImageFormats;
  nsCOMPtr<sbIMediaItem>  mItem;
  nsCOMPtr<imgIContainer> mImgContainer;
  nsCString               mImageMimeType;
  PRBool                  mHasAlbumArt;
  PRInt32                 mImageHeight;
  PRInt32                 mImageWidth;
};

#endif /*__SB_TRANSCODEALBUMART_H__*/

