/*=========================================================================

Library:   TubeTK

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/

#include "vtkSlicerTortuosityLogic.h"

// ITK includes
#include "itkVesselTubeSpatialObject.h"
#include "itktubeTortuositySpatialObjectFilter.h"

// Spatial object includes
#include "vtkMRMLSpatialObjectsNode.h"

// TubeTK includes
#include "tubeTubeMath.h"

// VTK includes
#include "vtkObjectFactory.h"
#include "vtkDelimitedTextWriter.h"
#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTable.h"

#include <math.h>

vtkStandardNewMacro(vtkSlicerTortuosityLogic);

//------------------------------------------------------------------------------
vtkSlicerTortuosityLogic::vtkSlicerTortuosityLogic( void )
{
  this->FlagToArrayNames[DistanceMetric] = "DistanceMetric";
  this->FlagToArrayNames[InflectionCountMetric] = "InflectionCountMetric";
  this->FlagToArrayNames[InflectionPoints] = "InflectionPoints";
  this->FlagToArrayNames[SumOfAnglesMetric] = "SumOfAnglesMetric";
}

//------------------------------------------------------------------------------
vtkSlicerTortuosityLogic::~vtkSlicerTortuosityLogic( void )
{
}

//------------------------------------------------------------------------------
void vtkSlicerTortuosityLogic::PrintSelf(ostream& os, vtkIndent indent)
{
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic::UniqueMeasure(int flag)
{
  return flag == vtkSlicerTortuosityLogic::DistanceMetric ||
    flag == vtkSlicerTortuosityLogic::InflectionCountMetric ||
    flag == vtkSlicerTortuosityLogic::InflectionPoints ||
    flag == vtkSlicerTortuosityLogic::SumOfAnglesMetric;
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkSlicerTortuosityLogic
::GetDistanceMetricArray(vtkMRMLSpatialObjectsNode* node, int flag)
{
  return this->GetOrCreateArray(
    node, flag & vtkSlicerTortuosityLogic::DistanceMetric);
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkSlicerTortuosityLogic
::GetInflectionCountMetricArray(vtkMRMLSpatialObjectsNode* node, int flag)
{
  return this->GetOrCreateArray(
    node, flag & vtkSlicerTortuosityLogic::InflectionCountMetric);
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkSlicerTortuosityLogic
::GetInflectionPointsArray(vtkMRMLSpatialObjectsNode* node, int flag)
{
  return this->GetOrCreateArray(
    node, flag & vtkSlicerTortuosityLogic::InflectionPoints);
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkSlicerTortuosityLogic
::GetSumOfAnglesMetricArray(vtkMRMLSpatialObjectsNode* node, int flag)
{
  return this->GetOrCreateArray(
    node, flag & vtkSlicerTortuosityLogic::SumOfAnglesMetric);
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkSlicerTortuosityLogic
::GetOrCreateArray(vtkMRMLSpatialObjectsNode* node, int flag)
{
  if (!flag || !this->UniqueMeasure(flag))
    {
    return NULL;
    }

  std::string name = this->FlagToArrayNames[flag];
  vtkDoubleArray* metricArray =
    this->GetOrCreateArray<vtkDoubleArray>(node, name.c_str());
  if (!metricArray)
    {
    return NULL;
    }

  // If it's new, make it the correct size
  vtkDoubleArray* ids = this->GetArray<vtkDoubleArray>(node, "TubeIDs");
  assert(ids);
  if (metricArray->GetSize() != ids->GetSize());
    {
    metricArray->SetNumberOfValues(ids->GetSize());
    }
  return metricArray;
}

//------------------------------------------------------------------------------
template<typename T> T* vtkSlicerTortuosityLogic
::GetArray(vtkMRMLSpatialObjectsNode* node, const char* name)
{
  vtkPolyData* polydata = node->GetPolyData();
  if (!polydata)
    {
    return NULL;
    }
  vtkPointData* pointData = polydata->GetPointData();
  if (!pointData)
    {
    return NULL;
    }

  return T::SafeDownCast(pointData->GetArray(name));
}

//------------------------------------------------------------------------------
template<typename T> T* vtkSlicerTortuosityLogic
::GetOrCreateArray(vtkMRMLSpatialObjectsNode* node, const char* name)
{
  T* metricArray = this->GetArray<T>(node, name);
  if (!metricArray)
    {
    vtkPolyData* polydata = node->GetPolyData();
    if (!polydata)
      {
      return NULL;
      }
    vtkPointData* pointData = polydata->GetPointData();
    if (!pointData)
      {
      return NULL;
      }

    vtkNew<T> newMetricArray;
    newMetricArray->SetName(name);
    pointData->AddArray(newMetricArray.GetPointer());
    return newMetricArray.GetPointer();
    }
  return metricArray;
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic
::RunDistanceMetric(vtkMRMLSpatialObjectsNode* node)
{
  return this->RunMetrics(
    node, vtkSlicerTortuosityLogic::DistanceMetric);
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic
::RunInflectionCountMetric(vtkMRMLSpatialObjectsNode* node)
{
  return this->RunMetrics(
    node, vtkSlicerTortuosityLogic::InflectionCountMetric);
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic
::RunInflectionPoints(vtkMRMLSpatialObjectsNode* node)
{
  return this->RunMetrics(
    node, vtkSlicerTortuosityLogic::InflectionPoints);
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic
::RunSumOfAnglesMetric(vtkMRMLSpatialObjectsNode* node)
{
  return this->RunMetrics(
    node, vtkSlicerTortuosityLogic::SumOfAnglesMetric);
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic
::RunMetrics(vtkMRMLSpatialObjectsNode* node, int flag,
             tube::SmoothTubeFunctionEnum smoothingMethod, double smoothingScale,
             int subsampling)
{
  if (!node)
    {
    return false;
    }

  // 1 - Get the metric arrays
  vtkDoubleArray* dm = this->GetDistanceMetricArray(node, flag);
  vtkDoubleArray* icm = this->GetInflectionCountMetricArray(node, flag);
  vtkDoubleArray* ip = this->GetInflectionPointsArray(node, flag);
  vtkDoubleArray* soam = this->GetSumOfAnglesMetricArray(node, flag);

  if (!dm && !icm && !ip && !soam)
    {
    std::cerr<<"Tortuosity flag mode unknown."<<std::endl;
    return false;
    }

  // Rewrite number of points array everytime
  vtkIntArray* nop = this->GetOrCreateArray<vtkIntArray>(node, "NumberOfPoints");
  nop->Initialize();

  // 2 - Fill the metric arrays
  typedef vtkMRMLSpatialObjectsNode::TubeNetType                    TubeNetType;
  typedef itk::VesselTubeSpatialObject<3>                           VesselTubeType;
  typedef itk::tube::TortuositySpatialObjectFilter<VesselTubeType>  FilterType;

  TubeNetType* spatialObject = node->GetSpatialObject();

  char childName[] = "Tube";
  TubeNetType::ChildrenListType* tubeList =
    spatialObject->GetChildren(spatialObject->GetMaximumDepth(), childName);

  size_t totalNumberOfPointsAdded = 0;
  for(TubeNetType::ChildrenListType::iterator tubeIt = tubeList->begin();
        tubeIt != tubeList->end(); ++tubeIt)
    {
    VesselTubeType* currTube =
      dynamic_cast<VesselTubeType*>((*tubeIt).GetPointer());
    if (!currTube)
      {
      continue;
      }

    if (currTube->GetNumberOfPoints() < 2)
      {
      std::cerr<<"Error, vessel #"<<currTube->GetId()
        <<" has less than 2 points !"<<std::endl
        <<"Skipping the vessel."<<std::endl;
      continue;
      }

    VesselTubeType::Pointer smoothedTube = VesselTubeType::New();
    VesselTubeType::Pointer subsampledTube = VesselTubeType::New();

    // Smooth the vessel
    smoothedTube = tube::SmoothTube<VesselTubeType>(currTube, smoothingScale, (tube::SmoothTubeFunctionEnum)smoothingMethod );
    // Subsample the vessel
    subsampledTube = tube::SubsampleTube<VesselTubeType>(smoothedTube, subsampling);

    // Update filter
    FilterType::Pointer filter = FilterType::New();
    filter->SetMeasureFlag(flag);
    filter->SetInput(subsampledTube);
    filter->Update();

    if (filter->GetOutput()->GetId() != subsampledTube->GetId())
      {
      std::cerr<<"Error while running filter on tube."<<std::endl;
      return false;
      }

    // Fill the arrays
    size_t numberOfPoints = currTube->GetPoints().size();
    for(size_t filterIndex = 0, tubeIndex = totalNumberOfPointsAdded;
      filterIndex < numberOfPoints; ++filterIndex, ++tubeIndex)
      {
      if (dm)
        {
        dm->SetValue(tubeIndex, filter->GetDistanceMetric());
        }
      if (icm)
        {
        icm->SetValue(tubeIndex, filter->GetInflectionCountMetric());
        }
      if (soam)
        {
        soam->SetValue(tubeIndex, filter->GetSumOfAnglesMetric());
        }
      if (ip)
        {
        ip->SetValue(tubeIndex, filter->GetInflectionPointValue(filterIndex));
        }

      nop->InsertNextValue(numberOfPoints);
      }

    totalNumberOfPointsAdded += numberOfPoints;
    }

  return true;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkSlicerTortuosityLogic::GetNamesFromFlag(int flag)
{
  std::vector<std::string> names;
  for (int compareFlag = vtkSlicerTortuosityLogic::DistanceMetric;
    compareFlag <= vtkSlicerTortuosityLogic::SumOfAnglesMetric;
    compareFlag = compareFlag << 1)
    {
    if (flag & compareFlag)
      {
      names.push_back(this->FlagToArrayNames[compareFlag]);
      }
    }
  return names;
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic
::SaveAsCSV(vtkMRMLSpatialObjectsNode* node, const char* filename, int flag)
{
  if (!node || !filename)
    {
    return false;
    }

  // Get the metric arrays
  std::vector<vtkDoubleArray*> metricArrays;
  std::vector<std::string> names = this->GetNamesFromFlag(flag);
  for (std::vector<std::string>::iterator it = names.begin();
    it != names.end(); ++it)
    {
    vtkDoubleArray* metricArray =
      this->GetArray<vtkDoubleArray>(node, it->c_str());
    if (metricArray)
      {
      metricArrays.push_back(metricArray);
      }
    }

  // Make sure we have everything we need for export
  if (metricArrays.size() <= 0)
    {
    std::cout<<"No array found for given flag: "<<flag<<std::endl;
    return false;
    }

  vtkIntArray* numberOfPointsArray =
    this->GetArray<vtkIntArray>(node, "NumberOfPoints");
  if (!numberOfPointsArray)
    {
    std::cerr<<"Expected ''NumberOfPoints'' array on the node point data."
      <<std::endl<<"Cannot proceed."<<std::endl;
    return false;
    }

  // Create  the table. Each column has only one value per vessel
  // instead of one value per each point of the vessel.
  vtkNew<vtkTable> table;
  for(std::vector<vtkDoubleArray*>::iterator it = metricArrays.begin();
    it != metricArrays.end(); ++it)
    {
    vtkNew<vtkDoubleArray> newArray;
    newArray->SetName((*it)->GetName());

    for (int j = 0; j < numberOfPointsArray->GetNumberOfTuples(); j += numberOfPointsArray->GetValue(j))
      {
      newArray->InsertNextValue((*it)->GetValue(j));
      }

    table->AddColumn(newArray.GetPointer());
    }

  // Write out the table to file
  vtkNew<vtkDelimitedTextWriter> writer;
  writer->SetFileName(filename);

#if (VTK_MAJOR_VERSION < 6)
  writer->SetInput(table.GetPointer());
#else
  writer->SetInputData(table.GetPointer());
#endif

  return writer->Write();
}

//------------------------------------------------------------------------------
bool vtkSlicerTortuosityLogic::LoadColorsFromCSV(
  vtkMRMLSpatialObjectsNode *node, const char* filename)
{
  if (!node || !filename)
    {
    return false;
    }

  typedef vtkMRMLSpatialObjectsNode::TubeNetType  TubeNetType;
  typedef itk::VesselTubeSpatialObject<3>         VesselTubeType;

  // Load the table from file
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(filename);
  reader->SetFieldDelimiterCharacters(",");
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  reader->Update();
  vtkTable* colorTable = reader->GetOutput();
  if (!colorTable)
    {
    std::cerr<<"Error in reading CSV file"<<std::endl;
    return false;
    }

  // Check if table is valid
  if (colorTable->GetNumberOfColumns() != 2)
    {
    std::cerr<<"Expected 2 columns in CSV file."
      <<std::endl<<"Cannot proceed."<<std::endl;
    return false;
    }

  // Get the tube list of the spatial object
  TubeNetType* spatialObject = node->GetSpatialObject();
  char childName[] = "Tube";
  TubeNetType::ChildrenListType* tubeList =
    spatialObject->GetChildren(spatialObject->GetMaximumDepth(), childName);

  // Create a new data array in the node
  double defaultValue = 0.0;
  vtkDoubleArray* customColorScaleArray =
    this->GetOrCreateArray<vtkDoubleArray>(node, "CustomColorScale");

  // Set the size of the array
  vtkDoubleArray* ids = this->GetArray<vtkDoubleArray>(node, "TubeIDs");
  assert(ids);
  if (customColorScaleArray->GetNumberOfTuples() != ids->GetNumberOfTuples())
    {
    customColorScaleArray->SetNumberOfTuples(ids->GetNumberOfTuples());
    }

  // Initialize the array with the default value
  customColorScaleArray->FillComponent(0, defaultValue);

  // Iterate through tubeList
  size_t totalNumberOfPoints = 0;
  TubeNetType::ChildrenListType::iterator tubeIt;
  for (tubeIt = tubeList->begin(); tubeIt != tubeList->end(); ++tubeIt)
    {
    VesselTubeType* currTube =
      dynamic_cast<VesselTubeType*>((*tubeIt).GetPointer());
    if (!currTube)
      {
      continue;
      }
    if (currTube->GetNumberOfPoints() < 2)
      {
      std::cerr<<"Error, vessel #"<<currTube->GetId()
        <<" has less than 2 points !"<<std::endl
        <<"Skipping the vessel."<<std::endl;
      continue;
      }

    // Get the current tube ID
    int tubeId = currTube->GetId();
    vtkDebugMacro(<<"Tube ID "<<tubeId);

    // Look for the ID in the table and get the corresponding value
    double valueToAssign = 0.0; //Default value for not specified tubes
    int tubeIndex = -1;
    for (size_t i = 0; i < colorTable->GetNumberOfRows(); i++)
      {
      if (colorTable->GetValue(i, 0).ToInt() == tubeId)
        {
        tubeIndex = i;
        valueToAssign = colorTable->GetValue(tubeIndex, 1).ToDouble();
        vtkDebugMacro(<<" found in CSV : value = "<<valueToAssign);
        break;
        }
      }
    if (tubeIndex == -1)
      {
      vtkDebugMacro(<<" not found in the CSV file");
      totalNumberOfPoints += currTube->GetPoints().size();
      continue;
      }

    // Fill the array of that tube
    size_t numberOfPoints = currTube->GetPoints().size();
    for (size_t j = totalNumberOfPoints; j < totalNumberOfPoints + numberOfPoints; j++)
      {
      customColorScaleArray->SetValue(j, valueToAssign);
      }
    totalNumberOfPoints += numberOfPoints;
    }

    // Notify the array of the changes
    customColorScaleArray->Modified();

  return true;
}
