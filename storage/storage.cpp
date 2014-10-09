#include "storage.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"
#include "../platform/servers_list.hpp"
#include "../platform/settings.hpp"

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"
#include "../coding/url_encode.hpp"
#include "../coding/file_name_utils.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/bind.hpp"
#include "../std/sstream.hpp"


using namespace downloader;

namespace storage
{
  /*
  static string ErrorString(DownloadResultT res)
  {
    switch (res)
    {
    case EHttpDownloadCantCreateFile:
      return "File can't be created. Probably, you have no disk space available or "
                         "using read-only file system.";
    case EHttpDownloadFailed:
      return "Download failed due to missing or poor connection. "
                         "Please, try again later.";
    case EHttpDownloadFileIsLocked:
      return "Download can't be finished because file is locked. "
                         "Please, try again after restarting application.";
    case EHttpDownloadFileNotFound:
      return "Requested file is absent on the server.";
    case EHttpDownloadNoConnectionAvailable:
      return "No network connection is available.";
    case EHttpDownloadOk:
      return "Download finished successfully.";
    }
    return "Unknown error";
  }
  */

  Storage::QueuedCountry::QueuedCountry(Storage const & storage, TIndex const & index, TMapOptions opt)
    : m_index(index), m_init(opt), m_left(opt)
  {
    m_pFile = &(storage.CountryByIndex(index).GetFile());
    m_current = (m_init & TMapOptions::EMapOnly ? TMapOptions::EMapOnly : TMapOptions::ECarRouting);
  }

  void Storage::QueuedCountry::AddOptions(TMapOptions opt)
  {
    TMapOptions const arr[] = { TMapOptions::EMapOnly, TMapOptions::ECarRouting };
    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    {
      if (opt & arr[i] && !(m_init & arr[i]))
      {
        m_init |= arr[i];
        m_left |= arr[i];
      }
    }
  }

  bool Storage::QueuedCountry::MoveNextFile()
  {
    if (m_current == m_left)
      return false;

    if (m_current == TMapOptions::EMapOnly)
      m_left = m_current = TMapOptions::ECarRouting;
    else
      m_left = m_current = TMapOptions::EMapOnly;
    return true;
  }

  bool Storage::QueuedCountry::Correct()
  {
    if ((m_init & TMapOptions::ECarRouting) &&
        (m_pFile->GetFileSize(TMapOptions::ECarRouting) == 0))
    {
      if (m_init & TMapOptions::EMapOnly)
        m_init = m_left = m_current = TMapOptions::EMapOnly;
      else
        return false;
    }

    return true;
  }

  uint64_t Storage::QueuedCountry::GetDownloadSize() const
  {
    return m_pFile->GetRemoteSize(m_current);
  }

  LocalAndRemoteSizeT Storage::QueuedCountry::GetFullSize() const
  {
    LocalAndRemoteSizeT res(0, 0);
    TMapOptions const arr[] = { TMapOptions::EMapOnly, TMapOptions::ECarRouting };
    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    {
      if (m_init & arr[i])
      {
        res.first += m_pFile->GetFileSize(arr[i]);
        res.second += m_pFile->GetRemoteSize(arr[i]);
      }
    }
    return res;
  }

  string Storage::QueuedCountry::GetFileName() const
  {
    return m_pFile->GetFileWithExt(m_current);
  }

  string Storage::QueuedCountry::GetMapFileName() const
  {
    return m_pFile->GetFileWithExt(TMapOptions::EMapOnly);
  }


  Storage::Storage() : m_currentSlotId(0)
  {
    LoadCountriesFile(false);

    if (Settings::IsFirstLaunchForDate(121031))
    {
      Platform & pl = GetPlatform();
      string const dir = pl.WritableDir();

      // Delete all: .mwm.downloading; .mwm.downloading2; .mwm.resume; .mwm.resume2
      string const regexp = "\\" DATA_FILE_EXTENSION "\\.(downloading2?$|resume2?$)";

      Platform::FilesList files;
      pl.GetFilesByRegExp(dir, regexp, files);

      for (size_t j = 0; j < files.size(); ++j)
        FileWriter::DeleteFileX(dir + files[j]);
    }
  }

  void Storage::Init(TUpdateAfterDownload const & updateFn)
  {
    m_updateAfterDownload = updateFn;
  }

  CountriesContainerT const & NodeFromIndex(CountriesContainerT const & root, TIndex const & index)
  {
    // complex logic to avoid [] out_of_bounds exceptions
    if (index.m_group == TIndex::INVALID || index.m_group >= static_cast<int>(root.SiblingsCount()))
      return root;
    else
    {
      if (index.m_country == TIndex::INVALID || index.m_country >= static_cast<int>(root[index.m_group].SiblingsCount()))
        return root[index.m_group];
      if (index.m_region == TIndex::INVALID || index.m_region >= static_cast<int>(root[index.m_group][index.m_country].SiblingsCount()))
        return root[index.m_group][index.m_country];
      return root[index.m_group][index.m_country][index.m_region];
    }
  }

  Country const & Storage::CountryByIndex(TIndex const & index) const
  {
    return NodeFromIndex(m_countries, index).Value();
  }

  void Storage::GetGroupAndCountry(TIndex const & index, string & group, string & country) const
  {
    string fName = CountryByIndex(index).GetFile().GetFileWithoutExt();
    CountryInfo::FileName2FullName(fName);
    CountryInfo::FullName2GroupAndMap(fName, group, country);
  }

  size_t Storage::CountriesCount(TIndex const & index) const
  {
    return NodeFromIndex(m_countries, index).SiblingsCount();
  }

  string const & Storage::CountryName(TIndex const & index) const
  {
    return NodeFromIndex(m_countries, index).Value().Name();
  }

  string const & Storage::CountryFlag(TIndex const & index) const
  {
    return NodeFromIndex(m_countries, index).Value().Flag();
  }

  string Storage::CountryFileName(TIndex const & index, TMapOptions opt) const
  {
    return QueuedCountry(*this, index, opt).GetFileName();
  }

  string const & Storage::CountryFileNameWithoutExt(TIndex const & index) const
  {
    return CountryByIndex(index).GetFile().GetFileWithoutExt();
  }

  LocalAndRemoteSizeT Storage::CountrySizeInBytes(const TIndex & index, TMapOptions opt) const
  {
    return QueuedCountry(*this, index, opt).GetFullSize();
  }

  TStatus Storage::CountryStatus(TIndex const & index) const
  {
    // first, check if we already downloading this country or have in in the queue
    auto const found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      if (found == m_queue.begin())
        return TStatus::EDownloading;
      else
        return TStatus::EInQueue;
    }

    // second, check if this country has failed while downloading
    if (m_failedCountries.count(index) > 0)
      return TStatus::EDownloadFailed;

    return TStatus::EUnknown;
  }

  TStatus Storage::CountryStatusEx(TIndex const & index) const
  {
    TStatus res = CountryStatus(index);
    if (res == TStatus::EUnknown)
    {
      Country const & c = CountryByIndex(index);
      LocalAndRemoteSizeT const size = c.Size(TMapOptions::EMapOnly);

      if (size.first == 0)
        return TStatus::ENotDownloaded;

      if (size.second == 0)
        return TStatus::EUnknown;

      res = TStatus::EOnDisk;
      if (size.first != size.second)
      {
        /// @todo Do better version check, not just size comparison.

        // Additional check for .ready file.
        // Use EOnDisk status if it's good, or EOnDiskOutOfDate otherwise.
        Platform const & pl = GetPlatform();
        string const fName = c.GetFile().GetFileWithExt(TMapOptions::EMapOnly) + READY_FILE_EXTENSION;

        uint64_t sz = 0;
        if (!pl.GetFileSizeByFullPath(pl.WritablePathForFile(fName), sz) || sz != size.second)
          res = TStatus::EOnDiskOutOfDate;
      }
    }

    return res;
  }

  void Storage::CountryStatusEx(TIndex const & index, TStatus & status, TMapOptions & options) const
  {
    status = CountryStatusEx(index);

    if (status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate)
    {
      options = TMapOptions::EMapOnly;

      string const fName = QueuedCountry(*this, index, TMapOptions::ECarRouting).GetFileName();
      Platform const & pl = GetPlatform();
      if (pl.IsFileExistsByFullPath(pl.WritablePathForFile(fName)))
        options |= TMapOptions::ECarRouting;
    }
  }

  void Storage::DownloadCountry(TIndex const & index, TMapOptions opt)
  {
    // check if we already downloading this country
    auto const found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      found->AddOptions(opt);
      return;
    }

    m_guideManager.UpdateGuidesData();

    // remove it from failed list
    m_failedCountries.erase(index);

    // add it into the queue
    QueuedCountry cnt(*this, index, opt);
    if (!cnt.Correct())
      return;
    m_queue.push_back(QueuedCountry(*this, index, opt));

    // and start download if necessary
    if (m_queue.size() == 1)
    {
      DownloadNextCountryFromQueue();
    }
    else
    {
      // notify about "In Queue" status
      NotifyStatusChanged(index);
    }
  }

  void Storage::NotifyStatusChanged(TIndex const & index)
  {
    for (CountryObservers const & o : m_observers)
      o.m_changeCountryFn(index);
  }

  void Storage::DownloadNextCountryFromQueue()
  {
    if (!m_queue.empty())
    {
      QueuedCountry & cnt = m_queue.front();

      m_countryProgress.first = 0;
      m_countryProgress.second = cnt.GetFullSize().second;

      DownloadNextFile(cnt);

      // new status for country, "Downloading"
      NotifyStatusChanged(cnt.GetIndex());
    }
  }

  void Storage::DownloadNextFile(QueuedCountry const & cnt)
  {
    // send Country name for statistics
    m_request.reset(HttpRequest::PostJson(GetPlatform().MetaServerUrl(),
            cnt.GetFileName(),
            bind(&Storage::OnServerListDownloaded, this, _1)));
  }

  /*
  m2::RectD Storage::CountryBounds(TIndex const & index) const
  {
    Country const & country = CountryByIndex(index);
    return country.Bounds();
  }
  */

  bool Storage::DeleteFromDownloader(TIndex const & index)
  {
    // check if we already downloading this country
    auto const found = find(m_queue.begin(), m_queue.end(), index);
    if (found != m_queue.end())
    {
      if (found == m_queue.begin())
      {
        // stop download
        m_request.reset();
        // remove from the queue
        m_queue.erase(found);
        // start another download if the queue is not empty
        DownloadNextCountryFromQueue();
      }
      else
      {
        // remove from the queue
        m_queue.erase(found);
      }

      return true;
    }

    return false;
  }

  bool Storage::IsDownloadInProgress() const
  {
    return !m_queue.empty();
  }

  void Storage::LoadCountriesFile(bool forceReload)
  {
    if (forceReload)
      m_countries.Clear();

    if (m_countries.SiblingsCount() == 0)
    {
      string json;
      ReaderPtr<Reader>(GetPlatform().GetReader(COUNTRIES_FILE)).ReadAsString(json);
      m_currentVersion = LoadCountries(json, m_countries);
      if (m_currentVersion < 0)
        LOG(LERROR, ("Can't load countries file", COUNTRIES_FILE));
    }
  }

  int Storage::Subscribe(TChangeCountryFunction const & change,
                         TProgressFunction const & progress)
  {
    CountryObservers obs;

    obs.m_changeCountryFn = change;
    obs.m_progressFn = progress;
    obs.m_slotId = ++m_currentSlotId;

    m_observers.push_back(obs);

    return obs.m_slotId;
  }

  void Storage::Unsubscribe(int slotId)
  {
    for (auto i = m_observers.begin(); i != m_observers.end(); ++i)
    {
      if (i->m_slotId == slotId)
      {
        m_observers.erase(i);
        return;
      }
    }
  }

  void Storage::OnMapDownloadFinished(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("queue can't be empty") );
      return;
    }

    QueuedCountry & cnt = m_queue.front();
    TIndex const index = cnt.GetIndex();

    if (request.Status() == HttpRequest::EFailed)
    {
      // add to failed countries set
      m_failedCountries.insert(index);
    }
    else
    {
      HttpRequest::ProgressT const & p = request.Progress();
      ASSERT_EQUAL(p.first, p.second, ());
      ASSERT_EQUAL(p.first, cnt.GetDownloadSize(), ());

      m_countryProgress.first += p.first;
      if (cnt.MoveNextFile())
      {
        DownloadNextFile(cnt);
        return;
      }

      // notify framework that downloading is done
      m_updateAfterDownload(cnt.GetMapFileName(), cnt.GetInitOptions());
    }

    m_queue.pop_front();

    NotifyStatusChanged(index);

    m_request.reset();
    DownloadNextCountryFromQueue();
  }

  void Storage::ReportProgress(TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    for (CountryObservers const & o : m_observers)
      o.m_progressFn(idx, p);
  }

  void Storage::OnMapDownloadProgress(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("queue can't be empty") );
      return;
    }

    if (!m_observers.empty())
    {
      HttpRequest::ProgressT p = request.Progress();
      p.first += m_countryProgress.first;
      p.second = m_countryProgress.second;

      ReportProgress(m_queue.front().GetIndex(), p);
    }
  }

  void Storage::OnServerListDownloaded(HttpRequest & request)
  {
    if (m_queue.empty())
    {
      ASSERT ( false, ("queue can't be empty") );
      return;
    }

    QueuedCountry const & cnt = m_queue.front();

    vector<string> urls;
    GetServerListFromRequest(request, urls);

    // append actual version and file name
    string const fileName = cnt.GetFileName();
    for (size_t i = 0; i < urls.size(); ++i)
      urls[i] = GetFileDownloadUrl(urls[i], fileName);

    string const filePath = GetPlatform().WritablePathForFile(fileName + READY_FILE_EXTENSION);
    m_request.reset(HttpRequest::GetFile(urls, filePath, cnt.GetDownloadSize(),
                                         bind(&Storage::OnMapDownloadFinished, this, _1),
                                         bind(&Storage::OnMapDownloadProgress, this, _1)));
    ASSERT ( m_request, () );
  }

  string Storage::GetFileDownloadUrl(string const & baseUrl, string const & fName) const
  {
    return baseUrl + OMIM_OS_NAME "/" + strings::to_string(m_currentVersion)  + "/" + UrlEncode(fName);
  }

  TIndex Storage::FindIndexByFile(string const & name) const
  {
    auto const isEqualFileName = [&name] (SimpleTree<Country> const & node)
    {
      Country const & c = node.Value();
      if (c.GetFilesCount() > 0)
        return (c.GetFile().GetFileWithoutExt() == name);
      else
        return false;
    };

    for (size_t i = 0; i < m_countries.SiblingsCount(); ++i)
    {
      if (isEqualFileName(m_countries[i]))
        return TIndex(i);

      for (size_t j = 0; j < m_countries[i].SiblingsCount(); ++j)
      {
        if (isEqualFileName(m_countries[i][j]))
          return TIndex(i, j);

        for (size_t k = 0; k < m_countries[i][j].SiblingsCount(); ++k)
        {
          if (isEqualFileName(m_countries[i][j][k]))
            return TIndex(i, j, k);
        }
      }
    }

    return TIndex();
  }

  void Storage::GetOutdatedCountries(vector<Country const *> & res) const
  {
    Platform & pl = GetPlatform();
    Platform::FilesList fList;
    pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, fList);

    for_each(fList.begin(), fList.end(), bind(&my::GetNameWithoutExt, _1));

    fList.erase(remove_if(fList.begin(), fList.end(), [] (string const & t)
    {
      return (t == WORLD_COASTS_FILE_NAME) || (t == WORLD_FILE_NAME);
    }), fList.end());

    fList.erase(remove_if(fList.begin(), fList.end(), [this] (string const & file)
    {
      return (CountryStatusEx(FindIndexByFile(file)) != TStatus::EOnDiskOutOfDate);
    }), fList.end());

    for (size_t i = 0; i < fList.size(); ++i)
      res.push_back(&CountryByIndex(FindIndexByFile(fList[i])));
  }
}
