#include "VectorMapSymbol.h"

const float OsmAnd::VectorMapSymbol::_absentElevation = -13e9f;

OsmAnd::VectorMapSymbol::VectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : MapSymbol(group_)
    , _verticesAndIndices(nullptr)
    , primitiveType(PrimitiveType::Invalid)
    , scaleType(ScaleType::Raw)
    , scale(1.0f)
{
}

OsmAnd::VectorMapSymbol::~VectorMapSymbol()
{
    releaseVerticesAndIndices();
}

OsmAnd::VectorMapSymbol::VerticesAndIndices::VerticesAndIndices()
: position31(nullptr)
, vertices(nullptr)
, verticesWithNormals(nullptr)
, verticesCount(0)
, indices(nullptr)
, indicesCount(0)
, partSizes(nullptr)
, zoomLevel(InvalidZoomLevel)
, isDenseObject(false)
{
}

OsmAnd::VectorMapSymbol::VerticesAndIndices::~VerticesAndIndices()
{
    if (position31 != nullptr)
    {
        delete position31;
        position31 = nullptr;
    }
    if (vertices != nullptr)
    {
        delete[] vertices;
        vertices = nullptr;
    }
    if (verticesWithNormals != nullptr)
    {
        delete[] verticesWithNormals;
        verticesWithNormals = nullptr;
    }
    verticesCount = 0;
    
    if (indices != nullptr)
    {
        delete[] indices;
        indices = nullptr;
    }
    indicesCount = 0;

    if (partSizes != nullptr)
        partSizes = nullptr;
    
    zoomLevel = InvalidZoomLevel;
    isDenseObject = false;
}

const std::shared_ptr<OsmAnd::VectorMapSymbol::VerticesAndIndices> OsmAnd::VectorMapSymbol::getVerticesAndIndices() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _verticesAndIndices;
}

void OsmAnd::VectorMapSymbol::setVerticesAndIndices(const std::shared_ptr<VerticesAndIndices>& verticesAndIndices)
{
    QWriteLocker scopedLocker(&_lock);
    
    _verticesAndIndices = verticesAndIndices;
}

void OsmAnd::VectorMapSymbol::releaseVerticesAndIndices()
{
    QWriteLocker scopedLocker(&_lock);

    _verticesAndIndices.reset();
}

void OsmAnd::VectorMapSymbol::generateCirclePrimitive(
    VectorMapSymbol& mapSymbol,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    if (pointsCount == 0)
    {
        mapSymbol.releaseVerticesAndIndices();
        return;
    }

    mapSymbol.primitiveType = PrimitiveType::TriangleFan;

    const auto verticesAndIndices = std::make_shared<VerticesAndIndices>();
    // Circle has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;

    // Allocate space for pointsCount+2 vertices
    verticesAndIndices->verticesCount = pointsCount + 2;
    verticesAndIndices->vertices = new Vertex[verticesAndIndices->verticesCount];
    auto pVertex = verticesAndIndices->vertices;

    // First vertex is the center
    pVertex->positionXYZD[0] = 0.0f;
    pVertex->positionXYZD[1] = _absentElevation;
    pVertex->positionXYZD[2] = 0.0f;
    pVertex->positionXYZD[3] = NAN;
    pVertex->color = color;
    pVertex += 1;

    // Generate each next vertex except the last one
    const auto step = (2.0*M_PI) / pointsCount;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto angle = step * pointIdx;
        pVertex->positionXYZD[0] = radius * qCos(angle);
        pVertex->positionXYZD[1] = _absentElevation;
        pVertex->positionXYZD[2] = radius * qSin(angle);
        pVertex->positionXYZD[3] = NAN;
        pVertex->color = color;

        pVertex += 1;
    }

    // Close the fan
    pVertex->positionXYZD[0] = verticesAndIndices->vertices[1].positionXYZD[0];
    pVertex->positionXYZD[1] = _absentElevation;
    pVertex->positionXYZD[2] = verticesAndIndices->vertices[1].positionXYZD[2];
    pVertex->positionXYZD[3] = NAN;
    pVertex->color = color;
    
    mapSymbol.setVerticesAndIndices(verticesAndIndices);
}

void OsmAnd::VectorMapSymbol::generateRingLinePrimitive(
    VectorMapSymbol& mapSymbol,
    const FColorARGB color /*= FColorARGB(1.0f, 1.0f, 1.0f, 1.0f)*/,
    const unsigned int pointsCount /*= 360*/,
    float radius /*= 1.0f*/)
{
    if (pointsCount == 0)
    {
        mapSymbol.releaseVerticesAndIndices();
        return;
    }

    mapSymbol.primitiveType = PrimitiveType::LineLoop;

    const auto verticesAndIndices = std::make_shared<VerticesAndIndices>();
    // Ring as line-loop has no reusable vertices, because it's rendered as triangle-fan,
    // so there's no indices
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;

    // Allocate space for pointsCount vertices
    verticesAndIndices->verticesCount = pointsCount;
    verticesAndIndices->vertices = new Vertex[verticesAndIndices->verticesCount];
    auto pVertex = verticesAndIndices->vertices;

    // Generate each vertex
    const auto step = (2.0*M_PI) / pointsCount;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto angle = step * pointIdx;
        pVertex->positionXYZD[0] = radius * qCos(angle);
        pVertex->positionXYZD[1] = _absentElevation;
        pVertex->positionXYZD[2] = radius * qSin(angle);
        pVertex->positionXYZD[3] = NAN;
        pVertex->color = color;

        pVertex += 1;
    }
    
    mapSymbol.setVerticesAndIndices(verticesAndIndices);
}

void OsmAnd::VectorMapSymbol::generateModel3DPrimitive(
    VectorMapSymbol& mapSymbol,
    const std::shared_ptr<const Model3D>& model3D,
    const QHash<QString, FColorARGB>& customMaterialColors)
{
    mapSymbol.primitiveType = PrimitiveType::Triangles;

    const auto verticesAndIndices = std::make_shared<VerticesAndIndices>();
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;

    verticesAndIndices->verticesCount = model3D->vertices.size();
    verticesAndIndices->verticesWithNormals = new VertexWithNormals[verticesAndIndices->verticesCount];
    auto pVertex = verticesAndIndices->verticesWithNormals;

    for (auto vertexIndex = 0u; vertexIndex < verticesAndIndices->verticesCount; vertexIndex++)
    {
        const auto& modelVertex = model3D->vertices[vertexIndex];

        pVertex->positionXYZ[0] = modelVertex.position[0];
        pVertex->positionXYZ[1] = modelVertex.position[1];
        pVertex->positionXYZ[2] = modelVertex.position[2];

        pVertex->normalXYZ[0] = modelVertex.normal[0];
        pVertex->normalXYZ[1] = modelVertex.normal[1];
        pVertex->normalXYZ[2] = modelVertex.normal[2];

        if (modelVertex.materialIndex == -1)
            pVertex->color = modelVertex.color;
        else
        {
            const auto& material = model3D->materials[modelVertex.materialIndex];
            const auto citCustomColor = customMaterialColors.constFind(material.name);
            pVertex->color = citCustomColor == customMaterialColors.cend() ? material.color : citCustomColor.value();
        }

        pVertex++;
    }
    verticesAndIndices->isDenseObject = true;

    mapSymbol.setVerticesAndIndices(verticesAndIndices);
}
