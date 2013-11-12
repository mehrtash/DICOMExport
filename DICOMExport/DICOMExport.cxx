#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkNumericSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"
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



  ImageIOType::Pointer gdcmIO = ImageIOType::New();

  /*
  NamesGeneratorType::Pointer namesGenerator = NamesGeneratorType::New();

  namesGenerator->SetInputDirectory( inputDirectory );

  const ReaderType::FileNamesContainer & filenames =
                            namesGenerator->GetInputFileNames();

  unsigned int numberOfFilenames =  filenames.size();
  std::cout<<"Number of fileNames:"<<std::endl;
  std::cout << numberOfFilenames << std::endl;

  for(unsigned int fni = 0; fni<numberOfFilenames; fni++)
    {
    std::cout << "filename # " << fni << " = ";
    std::cout << filenames[fni] << std::endl;
    }

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
*/

  // Reading Dictionary
  //typedef itk::MetaDataDictionary DictionaryType;

  /*
  const DictionaryType & dictionary = gdcmIO->GetMetaDataDictionary();

  typedef itk::MetaDataObject<std::string> MetaDataStringType;
  DictionaryType::ConstIterator itr = dictionary.Begin();
  DictionaryType::ConstIterator end = dictionary.End();

  while (itr != end)
  {
      itk::MetaDataObjectBase::Pointer entry = itr->second;
      MetaDataStringType::Pointer entryvalue =
              dynamic_cast<MetaDataStringType *>(entry.GetPointer());
      if (entryvalue)
      {
          std::string tagKey = itr->first;
          std::string tagValue = entryvalue->GetMetaDataObjectValue();
          std::cout<<tagKey<<"="<<tagValue<<std::endl;
      }
      ++itr;
  }

*/


  typename ReaderType::Pointer imageReader = ReaderType::New();
    try{
        imageReader->SetFileName(inputVolume.c_str());
        imageReader->Update();


    }
    catch(itk::ExceptionObject & excp){
        std::cerr<<"Exception thrown while reading the image file: "<<inputVolume<<std::endl;

    return EXIT_FAILURE;
    }


  ImageType::Pointer image = ImageType::New();
   image = imageReader->GetOutput();

   //  seriesWriter->
//  seriesWriter->SetInput( reader->GetOutput() );


   const char * outputDICOMDirectory = outputDirectory.c_str();
   itksys::SystemTools::MakeDirectory( outputDICOMDirectory );

   typedef signed short    OutputPixelType;
   const unsigned int      OutputDimension = 2;

   typedef itk::Image< OutputPixelType, OutputDimension >    Image2DType;

   typedef itk::ImageSeriesWriter<ImageType, Image2DType >  SeriesWriterType;

   SeriesWriterType::Pointer seriesWriter = SeriesWriterType::New();

   typedef itk::MetaDataDictionary DictionaryType;
   DictionaryType  outputDictionary ;
   itk::EncapsulateMetaData<std::string>(outputDictionary, "0010|0010", std::string("Gholoom") );
   typedef itk::NumericSeriesFileNames         NumericNamesGeneratorType;
   //image->SetMetaDataDictionary(outputDictionary);


   NumericNamesGeneratorType::Pointer outputNamesGenerator = NumericNamesGeneratorType::New();

   seriesWriter->SetInput(image);
   seriesWriter->SetImageIO( gdcmIO );

  std::string format = outputDirectory;

  format += "/image%03d.dcm";

  outputNamesGenerator->SetSeriesFormat( format.c_str() );

  //outputNamesGenerator->SetOutputDirectory( outputDirectory );

   ImageType::RegionType region =
   image->GetLargestPossibleRegion();

   ImageType::IndexType start = region.GetIndex();
   ImageType::SizeType  size  = region.GetSize();


   //outputNamesGenerator->SetSeriesFormat( format.c_str() );

   outputNamesGenerator->SetStartIndex( start[2] );
   outputNamesGenerator->SetEndIndex( start[2] + size[2] - 1 );
   outputNamesGenerator->SetIncrementIndex( 1 );
   seriesWriter->SetFileNames( outputNamesGenerator->GetFileNames() );
   // seriesWriter->SetMetaDataDictionaryArray(
     //                   reader->GetMetaDataDictionaryArray() );

  try
    {
    seriesWriter->Update();
    }
  catch( itk::ExceptionObject & excp )
    {
    std::cerr << "Exception thrown while writing the series " << std::endl;
    std::cerr << excp << std::endl;
    return EXIT_FAILURE;
    }


  return EXIT_SUCCESS;
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
