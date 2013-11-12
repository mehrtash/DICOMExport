#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include "itkShiftScaleImageFilter.h"
#include "itkGDCMImageIO.h"
#include "itkMetaDataObject.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkNumericSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include <vector>
#include "itksys/SystemTools.hxx"
#include "itkPluginUtilities.h"

#include "DICOMExportCLP.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{
std::string getTagValue(const std::string & entryID, const itk::MetaDataDictionary & inputDictionary);

template <class T>
int DoIt( int argc, char * argv[], T )
{
  PARSE_ARGS;
  typedef signed short    PixelType;
  const unsigned int      Dimension = 3;

  typedef    T InputPixelType;

  typedef itk::Image< PixelType, Dimension >      ImageType;
  typedef itk::ImageSeriesReader< ImageType >     ReaderType;

  typedef itk::GDCMImageIO                        ImageIOType;
  typedef itk::GDCMSeriesFileNames                NamesGeneratorType;

  typedef itk::Image<InputPixelType, 3>                        Image3DType;
  typedef itk::Image<InputPixelType, 2>                        Image2DType;

  typedef itk::ImageFileWriter<Image2DType>                    WriterType;

  ImageIOType::Pointer gdcmIO = ImageIOType::New();

  NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();

  namesGenerator->SetInputDirectory( inputDirectory );

  const ReaderType::FileNamesContainer & filenames =
                            namesGenerator->GetInputFileNames();

  ReaderType::Pointer reader = ReaderType::New();

  reader->SetImageIO( gdcmIO );
  reader->SetFileNames( filenames );

  try
    {
    reader->Update();
    }

  catch (itk::ExceptionObject &excp)
    {
    std::cerr << "Exception thrown while writing the image" << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
    }

  typedef itk::MetaDataDictionary DictionaryType;
  DictionaryType & inputDictionary =gdcmIO->GetMetaDataDictionary()  ;

  std::string  patientNameValue = getTagValue("0010|0010",inputDictionary);
  std::string  patientIDValue = getTagValue("0010|0020",inputDictionary);
  std::string  studyIDValue = getTagValue("0020|0010",inputDictionary);
  std::string  studyInstanceUIDValue = getTagValue("0020|000d",inputDictionary);


//----------------------------------------------------------------------------

  typedef itk::ImageFileReader<Image3DType>                    imageReaderType;
  typename Image3DType::Pointer image;
  typename imageReaderType::Pointer  imageReader = imageReaderType::New();
  try
      {
      imageReader->SetFileName(inputVolume.c_str() );
      imageReader->Update();
      image = imageReader->GetOutput();
      }
    catch( itk::ExceptionObject & excp )
      {
      std::cerr << "Exception thrown while reading the image file: " << inputVolume << std::endl;
      std::cerr << excp << std::endl;

      return EXIT_FAILURE;
      }

  //image = imageReader->GetOutput();

  // Image parameters

  unsigned int numberOfSlices = image->GetLargestPossibleRegion().GetSize()[2];

  typename Image3DType::SpacingType   spacing = image->GetSpacing();
  typename Image3DType::DirectionType oMatrix = image->GetDirection();

  DictionaryType       dictionary;

  // Populating dictionary for each 2D instance and writing to file

  for( unsigned int i = 0; i < numberOfSlices; i++ )
  {

  itksys_ios::ostringstream value;

  typename Image3DType::PointType    origin;
  typename Image3DType::IndexType    index;
  index.Fill(0);
  index[2] = i;
  image->TransformIndexToPhysicalPoint(index, origin);
  value.str("");
  value << origin[0] << "\\" << origin[1] << "\\" << origin[2];
  itk::EncapsulateMetaData<std::string>(dictionary, "0020|0032", value.str() );
  value.str("");
  value << i + 1;
  itk::EncapsulateMetaData<std::string>(dictionary, "0020|0013", value.str() );
  itk::EncapsulateMetaData<std::string>(dictionary, "0008|0008", std::string("ORIGINAL\\PRIMARY\\AXIAL") );  // Image Type
  itk::EncapsulateMetaData<std::string>(dictionary, "0008|0016", std::string("1.2.840.10008.5.1.4.1.1.2") ); // SOP Class UID
  value.str("");
  value << oMatrix[0][0] << "\\" << oMatrix[1][0] << "\\" << oMatrix[2][0] << "\\";
  value << oMatrix[0][1] << "\\" << oMatrix[1][1] << "\\" << oMatrix[2][1];
  itk::EncapsulateMetaData<std::string>(dictionary, "0020|0037", value.str() ); // Image Orientation (Patient)
  value.str("");
  value << spacing[2];
  itk::EncapsulateMetaData<std::string>(dictionary, "0018|0050", value.str() ); // Slice Thickness

  itk::EncapsulateMetaData<std::string>(dictionary, "0010|0010", patientNameValue.c_str() );
  itk::EncapsulateMetaData<std::string>(dictionary, "0010|0020", patientIDValue.c_str() );
  itk::EncapsulateMetaData<std::string>(dictionary, "0020|0010", studyIDValue.c_str() );
  itk::EncapsulateMetaData<std::string>(dictionary, "0020|000d", studyInstanceUIDValue.c_str() );
  typename Image3DType::RegionType extractRegion;
      typename Image3DType::SizeType   extractSize;
      typename Image3DType::IndexType  extractIndex;
      extractSize = image->GetLargestPossibleRegion().GetSize();
      extractIndex.Fill(0);
      extractIndex[2] = i;
      extractSize[2] = 0;
      extractRegion.SetSize(extractSize);
      extractRegion.SetIndex(extractIndex);

      typedef itk::ExtractImageFilter<Image3DType, Image2DType> ExtractType;
      typename ExtractType::Pointer extract = ExtractType::New();
  #if  ITK_VERSION_MAJOR >= 4
      extract->SetDirectionCollapseToGuess();  // ITKv3 compatible, but not recommended
  #endif
      extract->SetInput(image );
      extract->SetExtractionRegion(extractRegion);
      extract->GetOutput()->SetMetaDataDictionary(dictionary);
      extract->Update();

      itk::ImageRegionIterator<Image2DType> it( extract->GetOutput(), extract->GetOutput()->GetLargestPossibleRegion() );
      typename Image2DType::PixelType                minValue = itk::NumericTraits<typename Image2DType::PixelType>::max();
      typename Image2DType::PixelType                maxValue = itk::NumericTraits<typename Image2DType::PixelType>::min();
      for( it.GoToBegin(); !it.IsAtEnd(); ++it )
        {
        typename Image2DType::PixelType p = it.Get();
        if( p > maxValue )
          {
          maxValue = p;
          }
        if( p < minValue )
          {
          minValue = p;
          }
        }
      typename Image2DType::PixelType windowCenter = (minValue + maxValue) / 2;
      typename Image2DType::PixelType windowWidth = (maxValue - minValue);

      value.str("");
      value << windowCenter;
      itk::EncapsulateMetaData<std::string>(dictionary, "0028|1050", value.str() );
      value.str("");
      value << windowWidth;
      itk::EncapsulateMetaData<std::string>(dictionary, "0028|1051", value.str() );

      typename WriterType::Pointer writer = WriterType::New();
      char                imageNumber[BUFSIZ];
  #if WIN32
  #define snprintf sprintf_s
  #endif
      snprintf(imageNumber, BUFSIZ, "%04d", i + 1);
      value.str("");
      value << outputDirectory << "/" << "IMG" << imageNumber << ".dcm";
      writer->SetFileName(value.str().c_str() );

      writer->SetInput(extract->GetOutput() );
      //writer->SetUseCompression(useCompression);
      try
        {
        writer->SetImageIO(gdcmIO);
        writer->Update();
        }
      catch( itk::ExceptionObject & excp )
        {
        std::cerr << "Exception thrown while writing the file " << std::endl;
        std::cerr << excp << std::endl;
        return EXIT_FAILURE;
        }
  }



  return EXIT_SUCCESS;
}

std::string getTagValue(const std::string & entryId, const itk::MetaDataDictionary & inputDictionary)

{
  typedef itk::MetaDataDictionary DictionaryType;
  typedef itk::MetaDataObject< std::string > MetaDataStringType;

  DictionaryType::ConstIterator tagItr = inputDictionary.Find( entryId );
  DictionaryType::ConstIterator end = inputDictionary.End();

  std::string tagvalue;
  if( tagItr != end )
  {
    MetaDataStringType::ConstPointer entryvalue =
    dynamic_cast<const MetaDataStringType *>(tagItr->second.GetPointer() );

    if( entryvalue )
   {
     tagvalue = entryvalue->GetMetaDataObjectValue();
     std::cout << "(" << entryId <<  ") ";
     std::cout << " is: " << tagvalue.c_str() << std::endl;
   }
  }

return tagvalue;
}


} // end of anonymous namespace


int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  try
    {
    itk::GetImageType(inputVolume, pixelType, componentType);

    // This filter handles all types on input, but only produces
    // signed types
    switch( componentType )
      {
      case itk::ImageIOBase::UCHAR:
        return DoIt( argc, argv, static_cast<unsigned char>(0) );
        break;
      case itk::ImageIOBase::CHAR:
        return DoIt( argc, argv, static_cast<char>(0) );
        break;
      case itk::ImageIOBase::USHORT:
        return DoIt( argc, argv, static_cast<unsigned short>(0) );
        break;
      case itk::ImageIOBase::SHORT:
        return DoIt( argc, argv, static_cast<short>(0) );
        break;
      case itk::ImageIOBase::UINT:
        return DoIt( argc, argv, static_cast<unsigned int>(0) );
        break;
      case itk::ImageIOBase::INT:
        return DoIt( argc, argv, static_cast<int>(0) );
        break;
      case itk::ImageIOBase::ULONG:
        return DoIt( argc, argv, static_cast<unsigned long>(0) );
        break;
      case itk::ImageIOBase::LONG:
        return DoIt( argc, argv, static_cast<long>(0) );
        break;
      case itk::ImageIOBase::FLOAT:
        return DoIt( argc, argv, static_cast<float>(0) );
        break;
      case itk::ImageIOBase::DOUBLE:
        return DoIt( argc, argv, static_cast<double>(0) );
        break;
      case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
      default:
        std::cout << "unknown component type" << std::endl;
        break;
      }
    }

  catch( itk::ExceptionObject & excep )
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
