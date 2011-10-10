/*
 //
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

#ifndef ITUNESDATABASESERVICES_H_
#define ITUNESDATABASESERVICES_H_

#include <nsStringAPI.h>

#include "sbiTunesImporterCommon.h"

class sbIDatabasePreparedStatement;

/**
 * This class manages the ID mappings of the iTunes import table
 * in the database
 */
class sbiTunesDatabaseServices
{
public:
  /**
   * Initializes flags
   */
  sbiTunesDatabaseServices();
  // Need this so nsCOMPtr's are destructed in cpp
  ~sbiTunesDatabaseServices();
  /**
   * Performs initialization, such as creating the query objects
   */
  nsresult Initialize();
  /**
   * Map and iTunes ID to a Nightingale ID
   * \param aiTunesLibID the library ID of the iTunes item
   * \param aiTunesID the ID of the iTunes item
   * \param aNightingaleID the nightingale ID/guid
   */
  nsresult MapID(nsAString const & aiTunesLibID,
                 nsAString const & aiTunesID,
                 nsAString const & aNightingaleID);
  /**
   * Returns the Nightingale ID given an iTunes ID 
   * \param aiTunesLibID the library ID of the iTunes item
   * \param aiTunesID the ID of the iTunes item
   * \param aNightingaleID The returned Nightingale ID/GUID
   */
  nsresult GetSBIDFromITID(nsAString const & aiTunesLibID,
                           nsAString const & aiiTunesID,
                           nsAString & aNightingaleID);
  /**
   * Returns a given ID mapping based on the Nightingale ID
   * \param aNightingaleID the Nightingale ID of the mapping to be removed
   */
  nsresult RemoveSBIDEntry(nsAString const & aNightingaleID);
private:
  typedef nsCOMPtr<sbIDatabasePreparedStatement> PreparedStatementPtr;
  
  /**
   * Synchronous query object
   */
  sbIDatabaseQueryPtr mDBQuery;
  
  /**
   * Insert prepared statement
   */
  PreparedStatementPtr mInsertMapID;
  
  /**
   * Select prepared statement
   */
  PreparedStatementPtr mSelectMapID;
  
  /**
   * Delete prepared statement
   */
  PreparedStatementPtr mDeleteMapID;
  
  /**
   * Flag to denote that a reset is pending
   */
  PRBool mResetPending;
  
  // Prevent copy/assignment
  sbiTunesDatabaseServices(sbiTunesDatabaseServices const &);
  sbiTunesDatabaseServices & operator =(sbiTunesDatabaseServices const &);
};

#endif /* ITUNESDATABASESERVICES_H_ */
