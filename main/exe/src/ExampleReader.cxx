#include "eudaq/FileReader.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/StdEventConverter.hh"

#include <iostream>

static const std::string EVENT_TYPE = "Example";
static const uint32_t EVENT_ID = eudaq::cstr2hash("Example");

int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Example File Reader", "1.0",
      "Just an example, modify it to suit your own needs",
      1);
  eudaq::OptionFlag doraw(op, "r", "raw", "Display raw data from events");
  eudaq::OptionFlag docon(op, "c", "converted", "Display converted events");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);

    eudaq::Configuration conf;
    // Loop over all filenames
    for (size_t i = 0; i < op.NumArgs(); ++i) {

      // Create a reader for this file
      eudaq::FileReaderUP reader_up = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::cstr2hash("RawFileReader"), op.GetArg(i));
      eudaq::FileReader &reader = *(reader_up.get());
      
      // Display the actual filename (argument could have been a run number)
      std::cout << "Opened file: " << reader.Filename() << std::endl;

      // The BORE is now accessible in reader.GetDetectorEvent()
      if (docon.IsSet()) {
        // The PluginManager should be initialized with the BORE
        // eudaq::PluginManager::Initialize(*dynamic_cast<const eudaq::DetectorEvent*>(reader.GetNextEvent().get()));
      }

      // Now loop over all events in the file
      while (reader.NextEvent()) {
        if (reader.GetNextEvent()->IsEORE()) {
          std::cout << "End of run detected" << std::endl;
          // Don't try to process if it is an EORE
          break;
        }

        if (doraw.IsSet()) {
          // Display summary of raw event
          //std::cout << reader.GetDetectorEvent() << std::endl;

          try {
            // Look for a specific RawDataEvent, will throw an exception if not found
	    uint32_t nev = reader.GetNextEvent()->GetNumSubEvent();
	    for(uint32_t i= 0; i< nev;i++){
	      auto ev = reader.GetNextEvent()->GetSubEvent(i);
	      if(!ev&&(ev->GetEventID()==EVENT_ID)){
		// Display summary of the Example RawDataEvent
		ev->Print(std::cout);
	      }
	    }
          } catch (const eudaq::Exception & ) {
            std::cout << "No " << EVENT_TYPE << " subevent in event "
              << reader.GetNextEvent()->GetEventNumber()
              << std::endl;
          }
        }

        if (docon.IsSet()) {
	  auto ev_sp =  eudaq::StdEventConverter::MakeSharedStdEvent(0, 0);
	  eudaq::StdEventConverter::Convert(reader.GetNextEvent(), ev_sp, conf);
	  ev_sp->Print(std::cout);
	}
      }
    }

  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
