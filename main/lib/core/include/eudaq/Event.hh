#ifndef EUDAQ_INCLUDED_Event
#define EUDAQ_INCLUDED_Event

#include <string>
#include <vector>
#include <map>
#include <ostream>

#include "Serializable.hh"
#include "Serializer.hh"
#include "Exception.hh"
#include "Utils.hh"
#include "Platform.hh"
#include "Factory.hh"

namespace eudaq {
  class Event;
  using DetectorEvent = Event;

#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<Event>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)()>&
  Factory<Event>::Instance<>();
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(const uint32_t&, const uint32_t&, const uint32_t&)>&
  Factory<Event>::Instance<const uint32_t&, const uint32_t&, const uint32_t&>();

  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(const std::string&, const uint32_t&, const uint32_t&, const uint32_t&)>&
  Factory<Event>::Instance<const std::string&, const uint32_t&, const uint32_t&, const uint32_t&>();
  
#endif

  using EventUP = Factory<Event>::UP_BASE; 
  using EventSP = Factory<Event>::SP_BASE;
  using EventSPC = Factory<Event>::SPC_BASE;

  class DLLEXPORT Event : public Serializable{
  public:
    enum Flags {
      FLAG_BORE = 1, 
      FLAG_EORE = 2, 
      FLAG_HITS = 4, 
      FLAG_FAKE = 8, 
      FLAG_SIMU = 16, 
      FLAG_EUDAQ2 = 32, 
      FLAG_PACKET = 64,
      FLAG_BROKEN = 128,
      FLAG_STATUS = 256,
      FLAG_ALL = (unsigned)-1
    };

    Event();
    Event(const uint32_t type, const uint32_t run_n, const uint32_t stm_n);
    Event(Deserializer & ds);
    virtual void Serialize(Serializer &) const;   
    EventUP Clone() const;

    virtual void Print(std::ostream & os, size_t offset = 0) const;

    bool HasTag(const std::string &name) const {return m_tags.find(name) != m_tags.end();}
    void SetTag(const std::string &name, const std::string &val) {m_tags[name] = val;}
    const std::map<std::string, std::string>& GetTags() const {return m_tags;}
    std::string GetTag(const std::string &name, const std::string &def = "") const;
    std::string GetTag(const std::string &name, const char *def) const {return GetTag(name, std::string(def));}
    template <typename T> T GetTag(const std::string & name, T def) const {
      return eudaq::from_string(GetTag(name), def);
    }
    template <typename T> void SetTag(const std::string &name, const T &val) {
      SetTag(name, eudaq::to_string(val));
    }
    
    void SetFlagBit(uint32_t f) { m_flags |= f;}
    void ClearFlagBit(uint32_t f) { m_flags &= ~f;}
    bool IsFlagBit(uint32_t f) const { return (m_flags&f) == f;}

    bool IsBORE() const { return IsFlagBit(FLAG_BORE);}
    bool IsEORE() const { return IsFlagBit(FLAG_EORE);}
    void SetBORE() {SetTimestamp(0, 1); SetFlagBit(FLAG_BORE);}
    void SetEORE() {SetTimestamp(-2, -1); SetFlagBit(FLAG_EORE);}
    
    void AddSubEvent(EventSPC ev){m_sub_events.push_back(ev);}
    uint32_t GetNumSubEvent() const {return m_sub_events.size();}
    EventSPC GetSubEvent(uint32_t i) const {return m_sub_events.at(i);}
    
    void SetEventID(uint32_t id){m_type = id;}
    void SetVersion(uint32_t v){m_version = v;}
    void SetFlag(uint32_t f) {m_flags = f;}
    void SetRunN(uint32_t n){m_run_n = n;}
    void SetEventN(uint32_t n){m_ev_n = n;}
    void SetStreamN(uint32_t n){m_stm_n = n;}
    void SetTimestampBegin(uint64_t t){m_ts_begin = t; if(!m_ts_end) m_ts_end = t+1;}
    void SetTimestampEnd(uint64_t t){m_ts_end = t;}
    void SetTimestamp(uint64_t tb, uint64_t te){m_ts_begin = tb; m_ts_end = te;}
    void SetSubType(const std::string &t) {SetTag("SubType", t);}
 
    uint32_t GetEventID() const {return m_type;};
    uint32_t GetVersion()const {return m_version;}
    uint32_t GetFlag() const { return m_flags;}
    uint32_t GetRunN()const {return m_run_n;}
    uint32_t GetEventN()const {return m_ev_n;}
    uint32_t GetStreamN() const {return m_stm_n;}
    uint64_t GetTimestampBegin() const {return m_ts_begin;}
    uint64_t GetTimestampEnd() const {return m_ts_end;}
    std::string GetSubType() const {return GetTag("SubType");}

    static EventSP MakeShared(Deserializer&);
    
    // /////TODO: remove compatiable fun from EUDAQv1
    uint32_t GetEventNumber()const {return m_ev_n;}
    uint32_t GetRunNumber()const {return m_run_n;}

  private:
    uint32_t m_type;
    uint32_t m_version;
    uint32_t m_flags;
    uint32_t m_stm_n;
    uint32_t m_run_n;
    uint32_t m_ev_n;
    uint64_t m_ts_begin;
    uint64_t m_ts_end;
    std::map<std::string, std::string> m_tags;
    std::vector<EventSPC> m_sub_events;
  };
}

#endif // EUDAQ_INCLUDED_Event
