#include "WeatherTileResourceProvider.h"
#include "WeatherTileResourceProvider_P.h"

#include "QtExtensions.h"
#include <QStandardPaths>
#include <QThreadPool>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "GeoTileRasterizer.h"
#include "QRunnableFunctor.h"

#include <Logging.h>

OsmAnd::WeatherTileResourceProvider::WeatherTileResourceProvider(
    const QDateTime& dateTime,
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings,
    const QString& localCachePath,
    const QString& projResourcesPath,
    const uint32_t tileSize /*= 256*/,
    const float densityFactor /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new WeatherTileResourceProvider_P(this, dateTime, bandSettings, localCachePath, projResourcesPath, tileSize, densityFactor, webClient))
    , networkAccessAllowed(true)
{
}

OsmAnd::WeatherTileResourceProvider::~WeatherTileResourceProvider()
{
}

void OsmAnd::WeatherTileResourceProvider::obtainValue(
    const ValueRequest& request,
    const ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainValue(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourceProvider::obtainValueAsync(
    const ValueRequest& request,
    const ObtainValueAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainValueAsync(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourceProvider::obtainData(
    const TileRequest& request,
    const ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainData(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourceProvider::obtainDataAsync(
    const TileRequest& request,
    const ObtainTileDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->obtainDataAsync(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourceProvider::downloadGeoTiles(
    const DownloadGeoTileRequest& request,
    const DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->downloadGeoTiles(request, callback, collectMetric);
}

void OsmAnd::WeatherTileResourceProvider::downloadGeoTilesAsync(
    const DownloadGeoTileRequest& request,
    const DownloadGeoTilesAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    _p->downloadGeoTilesAsync(request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourceProvider::getGeoTileZoom()
{
    return ZoomLevel4;
}

OsmAnd::ZoomLevel OsmAnd::WeatherTileResourceProvider::getTileZoom(const WeatherLayer layer)
{
    switch (layer)
    {
        case WeatherLayer::Low:
            return ZoomLevel4;
        case WeatherLayer::High:
            return ZoomLevel7;
        default:
            return ZoomLevel4;
    }
}

OsmAnd::WeatherLayer OsmAnd::WeatherTileResourceProvider::getWeatherLayerByZoom(const ZoomLevel zoom)
{
    
    int lowZoom = WeatherTileResourceProvider::getTileZoom(WeatherLayer::Low);
    int lowOverZoom = WeatherTileResourceProvider::getMaxMissingDataZoomShift(WeatherLayer::Low);
    int lowUnderZoom = WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(WeatherLayer::Low);
    if (zoom >= lowZoom - lowUnderZoom && zoom <= lowZoom + lowOverZoom)
        return WeatherLayer::Low;

    int highZoom = WeatherTileResourceProvider::getTileZoom(WeatherLayer::High);
    int highOverZoom = WeatherTileResourceProvider::getMaxMissingDataZoomShift(WeatherLayer::High);
    int highUnderZoom = WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(WeatherLayer::High);
    if (zoom >= highZoom - highUnderZoom && zoom <= highZoom + highOverZoom)
        return WeatherLayer::High;

    return WeatherLayer::Undefined;
}

int OsmAnd::WeatherTileResourceProvider::getMaxMissingDataZoomShift(const WeatherLayer layer)
{
    if (layer == WeatherLayer::Low)
        return 2;
    else if (layer == WeatherLayer::High)
        return 5;
    else
        return 0;
}

int OsmAnd::WeatherTileResourceProvider::getMaxMissingDataUnderZoomShift(const WeatherLayer layer)
{
    if (layer == WeatherLayer::Low)
        return 2;
    else if (layer == WeatherLayer::High)
        return 0;
    else
        return 0;
}

void OsmAnd::WeatherTileResourceProvider::setBandSettings(const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings)
{
    return _p->setBandSettings(bandSettings);
}

int OsmAnd::WeatherTileResourceProvider::getCurrentRequestVersion() const
{
    return _p->getCurrentRequestVersion();
}

bool OsmAnd::WeatherTileResourceProvider::closeProvider()
{
    return _p->closeProvider();
}

OsmAnd::WeatherTileResourceProvider::ValueRequest::ValueRequest()
    : point31(0, 0)
    , zoom(ZoomLevel::InvalidZoomLevel)
    , band(0)
{
}

OsmAnd::WeatherTileResourceProvider::ValueRequest::ValueRequest(const ValueRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourceProvider::ValueRequest::~ValueRequest()
{
}

void OsmAnd::WeatherTileResourceProvider::ValueRequest::copy(ValueRequest& dst, const ValueRequest& src)
{
    dst.point31 = src.point31;
    dst.zoom = src.zoom;
    dst.band = src.band;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider::ValueRequest> OsmAnd::WeatherTileResourceProvider::ValueRequest::clone() const
{
    return std::shared_ptr<ValueRequest>(new ValueRequest(*this));
}

OsmAnd::WeatherTileResourceProvider::TileRequest::TileRequest()
    : weatherType(WeatherType::Raster)
    , tileId(TileId::zero())
    , zoom(InvalidZoomLevel)
    , version(0)
    , ignoreVersion(false)
{
}

OsmAnd::WeatherTileResourceProvider::TileRequest::TileRequest(const TileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourceProvider::TileRequest::~TileRequest()
{
}

void OsmAnd::WeatherTileResourceProvider::TileRequest::copy(TileRequest& dst, const TileRequest& src)
{
    dst.weatherType = src.weatherType;
    dst.tileId = src.tileId;
    dst.zoom = src.zoom;
    dst.bands = src.bands;
    dst.queryController = src.queryController;
    dst.version = src.version;
    dst.ignoreVersion = src.ignoreVersion;
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider::TileRequest> OsmAnd::WeatherTileResourceProvider::TileRequest::clone() const
{
    return std::shared_ptr<TileRequest>(new TileRequest(*this));
}

OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::DownloadGeoTileRequest()
    : forceDownload(false)
{
}

OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::DownloadGeoTileRequest(const DownloadGeoTileRequest& that)
{
    copy(*this, that);
}

OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::~DownloadGeoTileRequest()
{
}

void OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::copy(DownloadGeoTileRequest& dst, const DownloadGeoTileRequest& src)
{
    dst.topLeft = src.topLeft;
    dst.bottomRight = src.bottomRight;
    dst.forceDownload = src.forceDownload;
    dst.queryController = src.queryController;
}

std::shared_ptr<OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest> OsmAnd::WeatherTileResourceProvider::DownloadGeoTileRequest::clone() const
{
    return std::shared_ptr<DownloadGeoTileRequest>(new DownloadGeoTileRequest(*this));
}

OsmAnd::WeatherTileResourceProvider::Data::Data(
    TileId tileId_,
    ZoomLevel zoom_,
    AlphaChannelPresence alphaChannelPresence_,
    float densityFactor_,
    sk_sp<const SkImage> image_,
    QHash<BandIndex, QList<Ref<GeoContour>>> contourMap_ /*= QHash<BandIndex, QList<Ref<GeoContour>>>()*/)
    : tileId(tileId_)
    , zoom(zoom_)
    , alphaChannelPresence(alphaChannelPresence_)
    , densityFactor(densityFactor_)
    , image(image_)
    , contourMap(contourMap_)
{
}

OsmAnd::WeatherTileResourceProvider::Data::~Data()
{
}
