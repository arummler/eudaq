#include "NiController.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <stdio.h>
#include <stdlib.h>
#include <memory>

using eudaq::RawDataEvent;

class NiProducer : public eudaq::Producer {
public:
  NiProducer(const std::string name, const std::string &runcontrol);
  void OnConfigure(const eudaq::Configuration &param) override final;
  void OnStartRun(unsigned param) override final;
  void OnStopRun() override final;
  void OnTerminate() override final;
  void Exec() override final;

private:
  uint32_t m_id_stream;
  unsigned m_run, m_ev;
  bool done, running, configure;
  std::shared_ptr<NiController> ni_control;

  unsigned int datalength1;
  unsigned int datalength2;
  unsigned int ConfDataLength;
  std::vector<unsigned char> ConfDataError;

  unsigned TriggerType;
  unsigned Det;
  unsigned Mode;
  unsigned NiVersion;
  unsigned NumBoards;
  unsigned FPGADownload;
  unsigned MimosaID[6];
  unsigned MimosaEn[6];
  bool OneFrame;
  bool NiConfig;
  
  unsigned char conf_parameters[10];
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<NiProducer, const std::string&, const std::string&>(eudaq::cstr2hash("NiProducer"));
}



NiProducer::NiProducer(const std::string name, const std::string &runcontrol)
  : eudaq::Producer(name, runcontrol), done(false), running(false){
  m_id_stream = eudaq::cstr2hash(name.c_str());
    
  configure = false;

  std::cout << "NI Producer was started successful " << std::endl;
}

void NiProducer::Exec() {
  do {
    if (!running) {
      eudaq::mSleep(50);
      continue;
    }
    if (running) {

      datalength1 = ni_control->DataTransportClientSocket_ReadLength("priv");
      std::vector<unsigned char> mimosa_data_0(datalength1);
      mimosa_data_0 =
	ni_control->DataTransportClientSocket_ReadData(datalength1);

      datalength2 = ni_control->DataTransportClientSocket_ReadLength("priv");
      std::vector<unsigned char> mimosa_data_1(datalength2);
      mimosa_data_1 =
	ni_control->DataTransportClientSocket_ReadData(datalength2);

      eudaq::RawDataEvent ev("TelRawDataEvent", m_id_stream, m_run, m_ev++);
      ev.AddBlock(0, mimosa_data_0);
      ev.AddBlock(1, mimosa_data_1);
      SendEvent(ev);
    }

  } while (!done);
}

void NiProducer::OnConfigure(const eudaq::Configuration &param) {

  unsigned char configur[5] = "conf";

  try {
    if (!configure) {
      ni_control = std::make_shared<NiController>();
      ni_control->GetProduserHostInfo();
      std::string addr = param.Get("NiIPaddr", "localhost");
      uint16_t port = param.Get("NiConfigSocketPort", 49248);
      ni_control->ConfigClientSocket_Open(addr, port);
      addr = param.Get("NiIPaddr", "localhost");
      port = param.Get("NiDataTransportSocketPort", 49250);
      ni_control->DatatransportClientSocket_Open(addr, port);
      std::cout << " " << std::endl;
      configure = true;
    }

    TriggerType = param.Get("TriggerType", 255);
    Det = param.Get("Det", 255);
    Mode = param.Get("Mode", 255);
    NiVersion = param.Get("NiVersion", 255);
    NumBoards = param.Get("NumBoards", 255);
    FPGADownload = param.Get("FPGADownload", 1);
    for (unsigned char i = 0; i < 6; i++) {
      MimosaID[i] = param.Get("MimosaID_" + std::to_string(i + 1), 255);
      MimosaEn[i] = param.Get("MimosaEn_" + std::to_string(i + 1), 255);
    }
    OneFrame = param.Get("OneFrame", 255);

    std::cout << "Configuring ...(" << param.Name() << ")" << std::endl;

    conf_parameters[0] = NiVersion;
    conf_parameters[1] = TriggerType;
    conf_parameters[2] = Det;
    conf_parameters[3] = MimosaEn[1];
    conf_parameters[4] = MimosaEn[2];
    conf_parameters[5] = MimosaEn[3];
    conf_parameters[6] = MimosaEn[4];
    conf_parameters[7] = MimosaEn[5];
    conf_parameters[8] = NumBoards;
    conf_parameters[9] = FPGADownload;

    ni_control->ConfigClientSocket_Send(configur, sizeof(configur));
    ni_control->ConfigClientSocket_Send(conf_parameters,
					sizeof(conf_parameters));

    ConfDataLength = ni_control->ConfigClientSocket_ReadLength("priv");
    ConfDataError = ni_control->ConfigClientSocket_ReadData(ConfDataLength);

    NiConfig = false;

    if ((ConfDataError[3] & 0x1) >> 0) {
      EUDAQ_ERROR("NI crate can not be configure: ErrorReceive Config");
      NiConfig = true;
    } // ErrorReceive Config
    if ((ConfDataError[3] & 0x2) >> 1) {
      EUDAQ_ERROR("NI crate can not be configure: Error FPGA open");
      NiConfig = true;
    } // Error FPGA open
    if ((ConfDataError[3] & 0x4) >> 2) {
      EUDAQ_ERROR("NI crate can not be configure: Error FPGA reset");
      NiConfig = true;
    } // Error FPGA reset
    if ((ConfDataError[3] & 0x8) >> 3) {
      EUDAQ_ERROR("NI crate can not be configure: Error FPGA download");
      NiConfig = true;
    } // Error FPGA download
    if ((ConfDataError[3] & 0x10) >> 4) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_0 Start");
      NiConfig = true;
    } // FIFO_0 Configure
    if ((ConfDataError[3] & 0x20) >> 5) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_1 Start");
      NiConfig = true;
    } // FIFO_0 Start
    if ((ConfDataError[3] & 0x40) >> 6) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_2 Start");
      NiConfig = true;
    } // FIFO_1 Configure
    if ((ConfDataError[3] & 0x80) >> 7) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_3 Start");
      NiConfig = true;
    } // FIFO_1 Start
    if ((ConfDataError[2] & 0x1) >> 0) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_4 Start");
      NiConfig = true;
    } // FIFO_2 Configure
    if ((ConfDataError[2] & 0x2) >> 1) {
      EUDAQ_ERROR("NI crate can not be configure: FIFO_5 Start");
      NiConfig = true;
    } // FIFO_2 Start

    if (NiConfig) {
      std::cout << "NI crate was Configured with ERRORs " << param.Name()
		<< " " << std::endl;
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } else {
      std::cout << "... was Configured " << param.Name() << " " << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    }
  } catch (const std::exception &e) {
    printf("Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
  } catch (...) {
    printf("Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
  }
}

void NiProducer::OnStartRun(unsigned param) {
  try {
    m_run = param;
    m_ev = 0;
    std::cout << "Start Run: " << param << std::endl;

    eudaq::RawDataEvent ev("TelRawDataEvent", m_id_stream, m_run, 0);
    ev.SetBORE();

      
    ev.SetTag("DET", "MIMOSA26");
    ev.SetTag("MODE", "ZS2");
    ev.SetTag("BOARDS", NumBoards);
    for (unsigned char i = 0; i < 6; i++)
      ev.SetTag("ID" + std::to_string(i), std::to_string(MimosaID[i]));
    for (unsigned char i = 0; i < 6; i++)
      ev.SetTag("MIMOSA_EN" + std::to_string(i), std::to_string(MimosaEn[i]));
    SendEvent(ev);
    eudaq::mSleep(500);

    ni_control->Start();
    running = true;

    SetStatus(eudaq::Status::LVL_OK, "Started");
  } catch (const std::exception &e) {
    printf("Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
  } catch (...) {
    printf("Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
  }
}

void NiProducer::OnStopRun() {
  try {
    std::cout << "Stop Run" << std::endl;

    ni_control->Stop();
    eudaq::mSleep(5000);
    running = false;
    eudaq::mSleep(100);
    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SetStatus(eudaq::Status::LVL_OK, "Stopped");

    eudaq::RawDataEvent ev("TelRawDataEvent", m_id_stream, m_run, m_ev);
    ev.SetEORE();
    SendEvent(ev);
      
  } catch (const std::exception &e) {
    printf("Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  } catch (...) {
    printf("Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}

void NiProducer::OnTerminate() {
  std::cout << "Terminate (press enter)" << std::endl;
  done = true;
  ni_control->DatatransportClientSocket_Close();
  ni_control->ConfigClientSocket_Close();
  eudaq::mSleep(1000);
}
