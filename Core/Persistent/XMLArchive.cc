
#include <Core/Persistent/Archive.h>
#include <Core/Persistent/ArchiveElement.h>
#include <Core/Exceptions/InputError.h>
#include <Core/Exceptions/SerializationError.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <map>
#include <sstream>
#include <string>
#include <cstring>

using namespace Manta;
using namespace std;

namespace /* Anonymous */ {
  struct RefInfo {
    xmlNodePtr node;
    std::string refname;
    int useCount;
  };
  class XMLArchive;

  class XMLArchiveElement : public ArchiveElement {
  public:
    enum ElementType {
      RootElement, NormalElement, TagElement, UsedTagElement, ContainerElement
    };
    XMLArchiveElement(XMLArchive* archive, xmlNodePtr node, ElementType element_type);
    XMLArchiveElement(XMLArchive* archive, xmlNodePtr node, const std::string& tagname);
    virtual ~XMLArchiveElement();

    virtual void readwrite(const std::string& fieldname, bool& data);
    virtual void readwrite(const std::string& fieldname, signed char& data);
    virtual void readwrite(const std::string& fieldname, unsigned char& data);
    virtual void readwrite(const std::string& fieldname, short& data);
    virtual void readwrite(const std::string& fieldname, unsigned short& data);
    virtual void readwrite(const std::string& fieldname, int& data);
    virtual void readwrite(const std::string& fieldname, unsigned int& data);
    virtual void readwrite(const std::string& fieldname, long& data);
    virtual void readwrite(const std::string& fieldname, unsigned long& data);
    virtual void readwrite(const std::string& fieldname, long long& data);
    virtual void readwrite(const std::string& fieldname, unsigned long long& data);
    virtual void readwrite(const std::string& fieldname, float& data);
    virtual void readwrite(const std::string& fieldname, double& data);
    virtual void readwrite(const std::string& fieldname, long double& data);

    virtual void readwrite(const std::string& fieldname, bool* data, int numElements);
    virtual void readwrite(const std::string& fieldname, signed char* data, int numElements);
    virtual void readwrite(const std::string& fieldname, unsigned char* data, int numElements);
    virtual void readwrite(const std::string& fieldname, short* data, int numElements);
    virtual void readwrite(const std::string& fieldname, unsigned short* data, int numElements);
    virtual void readwrite(const std::string& fieldname, int* data, int numElements);
    virtual void readwrite(const std::string& fieldname, unsigned int* data, int numElements);
    virtual void readwrite(const std::string& fieldname, long* data, int numElements);
    virtual void readwrite(const std::string& fieldname, unsigned long* data, int numElements);
    virtual void readwrite(const std::string& fieldname, long long* data, int numElements);
    virtual void readwrite(const std::string& fieldname, unsigned long long* data, int numElements);
    virtual void readwrite(const std::string& fieldname, float* data, int numElements);
    virtual void readwrite(const std::string& fieldname, double* data, int numElements);
    virtual void readwrite(const std::string& fieldname, long double* data, int numElements);

    virtual void readwrite(const std::string& fieldname, std::string& data);

    virtual void readwrite(const std::string& fieldname, PointerWrapperInterface& ptr, bool isPointer);

    virtual bool nextContainerElement();

    virtual bool hasField(const std::string& fieldname) const;
  protected:
    XMLArchive* archive;
    xmlNodePtr node;
    ElementType element_type;
    xmlNodePtr current_container_child;
    string tagname;
    void writeProperty(const std::string& fieldname, const std::string& value);
    std::string readProperty(const std::string& fieldname);

    template<class T>
    void readwriteInteger(const std::string& fieldname, T& data);
    template<class T>
    void readwriteIntegerArray(const std::string& fieldname, T* data, int numElements);
  };

  typedef std::map<void*, RefInfo> refmaptype;
  typedef std::map<std::string, int> countmaptype;
  typedef std::map<std::string, PointerWrapperInterface*> pointermaptype;
  typedef std::map<std::string, xmlNodePtr> assetmaptype;

  class XMLArchive : public Archive {
  public:
    XMLArchive(xmlDocPtr doc);
    XMLArchive(const std::string& filename);
    virtual ~XMLArchive();

    virtual ArchiveElement* getRoot();

    refmaptype refmap;
    countmaptype countmap;
    pointermaptype pointermap;
    assetmaptype assetmap;
  private:
    std::string filename;
    xmlDocPtr doc;
    void scanPointers(xmlNodePtr node);

    static bool force_initialize;
  };
}

static inline
const char* to_char_ptr(const xmlChar *t) {
  return (const char*)t;
}

static inline
const xmlChar* to_xml_ch_ptr(const char *t) {
  return (xmlChar*)t;
}

static int count(const std::string& str1, const std::string& str2)
{
  int count = 0;
  string::size_type index = 0;
  while((index = str1.find(str2, index)) != string::npos){
    index++;
    count++;
  }
  return count;
}

static void make_safe_classname(std::string& safe_classname, bool& have_complex_classname)
{
  // If this is a complex classname, we will use "template" as the element name
  // and put the classname in an attribute called "type"
  have_complex_classname = false;
  if(count(safe_classname, "<") > 1 || count(safe_classname, ",") > 0) {
    have_complex_classname = true;
    safe_classname = "template";
  } else {
    // This could be made considerably faster if it is ever a bottleneck
    string classname = safe_classname;
    string::size_type index = 0;
    while((index = safe_classname.find("::")) != string::npos)
      safe_classname.replace(index, 2, ".");
    while((index = safe_classname.find("<")) != string::npos)
      safe_classname.replace(index, 1, "-");
    while((index = safe_classname.find(">")) != string::npos)
      safe_classname.erase(index, 1);
    while((index = safe_classname.find(" ")) != string::npos)
      safe_classname.erase(index, 1);
  }
}

static std::string get_classname(xmlNodePtr node, const std::string& fieldname)
{
  if(strcmp(to_char_ptr(node->name), "template") == 0){
    xmlChar* templname = xmlGetProp(node, to_xml_ch_ptr("type"));
    if(!templname)
      throw SerializationError("Unspecified type for template: " + fieldname);
    return to_char_ptr(templname);
  } else {
    string classname = to_char_ptr(node->name);
    string::size_type index = 0;
    while((index = classname.find(".")) != string::npos)
      classname.replace(index, 1, "::");
    int closecount = 0;
    while((index = classname.find("-")) != string::npos){
      classname.replace(index, 1, "<");
      closecount++;
    }
    for(int i=0;i<closecount;i++)
      classname.push_back('>');
    return classname;
  }
}

static Archive* readopener(const std::string& filename)
{
  LIBXML_TEST_VERSION;

  xmlDocPtr doc = xmlReadFile(filename.c_str(), 0, XML_PARSE_PEDANTIC|XML_PARSE_XINCLUDE);

  /* check if parsing suceeded */
  if (doc == 0)
    throw InputError("Error reading file: "+filename);

  xmlNodePtr root = xmlDocGetRootElement(doc);
  if(strcmp(to_char_ptr(root->name), "Manta") != 0)
    return 0;

  return new XMLArchive(doc);
}

static Archive* writeopener(const std::string& filename)
{
  // Pull off the suffix...
  string::size_type dot = filename.rfind('.');
  string suffix;
  if(dot == string::npos){
    suffix="";
  } else {
    suffix = filename.substr(dot+1);
  }
  if(suffix == "xml" || suffix == "rtml"){
    return new XMLArchive(filename);
  } else {
    return 0;
  }
}

bool XMLArchive::force_initialize = Archive::registerArchiveType("xml", readopener, writeopener);

XMLArchive::XMLArchive(xmlDocPtr doc)
  : Archive(true), doc(doc)
{
}

XMLArchive::XMLArchive(const std::string& infilename)
  : Archive(false)
{
  doc = xmlNewDoc(to_xml_ch_ptr("1.0"));
  filename = infilename;
}

XMLArchive::~XMLArchive()
{
  if(writing()){
    xmlKeepBlanksDefault(0);
    xmlSaveFormatFileEnc(filename.c_str(), doc, "UTF-8", 1);
  }
  xmlFreeDoc(doc);
}

void XMLArchive::scanPointers(xmlNodePtr node)
{
  xmlChar* id = xmlGetProp(node, to_xml_ch_ptr("id"));
  if(id){
    string sid = to_char_ptr(id);
    assetmaptype::iterator iter = assetmap.find(sid);
    if(iter != assetmap.end())
      throw SerializationError("Pointer: " + sid + " defined twice");
    assetmap.insert(std::make_pair(sid, node));
  }
  for(xmlNodePtr child = node->children; child != 0; child = child->next){
    if(child->type == XML_ELEMENT_NODE)
      scanPointers(child);
  }
}

ArchiveElement* XMLArchive::getRoot()
{
  if(reading()){
    xmlNodePtr root = xmlDocGetRootElement(doc);
    scanPointers(root);
    return new XMLArchiveElement(this, root, XMLArchiveElement::RootElement);
  } else {
    xmlNodePtr top = xmlNewNode(0, to_xml_ch_ptr("Manta"));
    xmlDocSetRootElement(doc, top);
    return  new XMLArchiveElement(this, top, XMLArchiveElement::RootElement);
  }
}

XMLArchiveElement::XMLArchiveElement(XMLArchive* archive, xmlNodePtr node, ElementType element_type)
  : ArchiveElement(archive->reading()), archive(archive), node(node), element_type(element_type),
    current_container_child(0)
{
}

XMLArchiveElement::XMLArchiveElement(XMLArchive* archive, xmlNodePtr node, const std::string& tagname)
  : ArchiveElement(archive->reading()), archive(archive), node(node), element_type(TagElement), tagname(tagname)
{
}

XMLArchiveElement::~XMLArchiveElement()
{
}

void XMLArchiveElement::writeProperty(const std::string& fieldname, const std::string& value)
{
  if(element_type == UsedTagElement)
    throw SerializationError("Lightweight class written with more than one data member");
  else if(element_type == TagElement){
    element_type = UsedTagElement;
    if(xmlHasProp(node, to_xml_ch_ptr(tagname.c_str()))){
      ostringstream msg;
      msg << "Writing duplicate field (" << tagname << ") in writing class " << to_char_ptr(node->name);
      throw SerializationError(msg.str());
    }
    xmlSetProp(node, to_xml_ch_ptr(tagname.c_str()), to_xml_ch_ptr(value.c_str()));
  } else {
    if(xmlHasProp(node, to_xml_ch_ptr(fieldname.c_str()))){
      ostringstream msg;
      msg << "Writing duplicate field (" << fieldname << ") in writing class " << to_char_ptr(node->name);
      throw SerializationError(msg.str());
    }
    xmlSetProp(node, to_xml_ch_ptr(fieldname.c_str()), to_xml_ch_ptr(value.c_str()));
  }
}

std::string XMLArchiveElement::readProperty(const std::string& fieldname)
{
  if(element_type == UsedTagElement)
    throw SerializationError("Lightweight class read with more than one data member");
  if(element_type == TagElement){
    xmlChar* prop = xmlGetProp(node, to_xml_ch_ptr(tagname.c_str()));
    if(!prop)
      throw SerializationError(std::string("Missing tag: ") + to_char_ptr(prop));
    return to_char_ptr(prop);
  } else {
    xmlChar* prop = xmlGetProp(node, to_xml_ch_ptr(fieldname.c_str()));
    if(prop)
      return to_char_ptr(prop);

    xmlNode* child = node->children;
    while(child && child->type != XML_TEXT_NODE)
      child = child->next;
    if(!child)
      throw SerializationError("No data in element for field: " + fieldname);
    return to_char_ptr(XML_GET_CONTENT(child));
  }
}

template<class T>
void XMLArchiveElement::readwriteInteger(const std::string& fieldname, T& data)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    in >> data;
    if(!in)
      throw SerializationError(std::string("Error parsing ") + typeid(T).name() + ": " + in.str());
  } else {
    ostringstream datastring;
    datastring << data;
    writeProperty(fieldname, datastring.str());
  }
}

template<class T>
void XMLArchiveElement::readwriteIntegerArray(const std::string& fieldname, T* data, int numElements)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    for(int i=0;i<numElements;i++)
      in >> data[i];
    if(!in)
      throw SerializationError("Error parsing float array: " + in.str());
  } else {
    ostringstream datastring;
    for(int i=0;i<numElements;i++){
      if(i != 0)
        datastring << " ";
      datastring << data[i];
    }
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, bool& data)
{
  if(reading()){
    string str = readProperty(fieldname);
    if(str == "true" || str == "1" || str == "TRUE" || str == "yes")
      data = true;
    else if(str == "false" || str == "0" || str == "FALSE" || str == "no")
      data = false;
    else
      throw SerializationError("Unknown boolean value: " + str);
  } else {
    writeProperty(fieldname, data?"true":"false");
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, signed char& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned char& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, short& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned short& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, int& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned int& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, long& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned long& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, long long& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned long long& data)
{
  readwriteInteger(fieldname, data);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, float& data)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    in >> data;
    if(!in)
      throw SerializationError("Error parsing float: " + in.str());
  } else {
    ostringstream datastring;
    datastring.precision(8);
    datastring << data;
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, double& data)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    in >> data;
    if(!in)
      throw SerializationError("Error parsing float: " + in.str());
  } else {
    ostringstream datastring;
    datastring.precision(17);
    datastring << data;
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, long double& data)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    in >> data;
    if(!in)
      throw SerializationError("Error parsing float: " + in.str());
  } else {
    ostringstream datastring;
    datastring.precision(34);
    datastring << data;
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, bool* data, int numElements)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    for(int i=0;i<numElements;i++){
      string str;
      in >> str;
      if(!in)
        throw SerializationError("Error parsing float array: " + in.str());
      if(str == "true" || str == "1" || str == "TRUE" || str == "yes")
        data[i] = true;
      else if(str == "false" || str == "0" || str == "FALSE" || str == "no")
        data[i] = false;
      else
        throw SerializationError("Unknown boolean value: " + str);
    }
  } else {
    ostringstream datastring;
    for(int i=0;i<numElements;i++){
      if(i != 0)
        datastring << " ";
      datastring << (data[i]?"true":"false");
    }
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, signed char* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned char* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, short* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned short* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, int* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned int* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, long* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned long* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, long long* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, unsigned long long* data, int numElements)
{
  readwriteIntegerArray(fieldname, data, numElements);
}

void XMLArchiveElement::readwrite(const std::string& fieldname, float* data, int numelements)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    for(int i=0;i<numelements;i++)
      in >> data[i];
    if(!in)
      throw SerializationError("Error parsing float array: " + in.str());
  } else {
    ostringstream datastring;
    datastring.precision(8);
    for(int i=0;i<numelements;i++){
      if(i != 0)
        datastring << " ";
      datastring << data[i];
    }
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, double* data, int numelements)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    for(int i=0;i<numelements;i++)
      in >> data[i];
    if(!in)
      throw SerializationError("Error parsing double array: " + in.str());
  } else {
    ostringstream datastring;
    datastring.precision(17);
    for(int i=0;i<numelements;i++){
      if(i != 0)
        datastring << " ";
      datastring << data[i];
    }
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, long double* data, int numelements)
{
  if(reading()){
    istringstream in(readProperty(fieldname));
    for(int i=0;i<numelements;i++)
      in >> data[i];
    if(!in)
      throw SerializationError("Error parsing long double array: " + in.str());
  } else {
    ostringstream datastring;
    datastring.precision(34);
    for(int i=0;i<numelements;i++){
      if(i != 0)
        datastring << " ";
      datastring << data[i];
    }
    writeProperty(fieldname, datastring.str());
  }
}

void XMLArchiveElement::readwrite(const std::string& fieldname, std::string& data)
{
  if(reading()){
    data = readProperty(fieldname);
  } else {
    writeProperty(fieldname, data);
  }
}

bool XMLArchiveElement::nextContainerElement()
{
  if(reading()){
    if(!node)
      return false;
    if(!current_container_child)
      current_container_child = node->children;
    else
      current_container_child = current_container_child->next;
    while(current_container_child && current_container_child->type != XML_ELEMENT_NODE)
      current_container_child = current_container_child->next;
    if(current_container_child == 0){
      node = 0;
      return false;
    } else {
      return true;
    }
  } else {
    return true;
  }
}

bool XMLArchiveElement::hasField(const std::string& fieldname) const
{
  if(!reading())
    return true;

  if(xmlHasProp(node, to_xml_ch_ptr(fieldname.c_str())))
    return true;
  xmlNodePtr child = node->children;
  while(child && (child->type != XML_ELEMENT_NODE || strcmp(to_char_ptr(child->name), fieldname.c_str()) != 0))
    child = child->next;
  if(child)
    return true;

  return false;
}


void XMLArchiveElement::readwrite(const std::string& fieldname, PointerWrapperInterface& ptr, bool isPointer)
{
  if(reading()){
    if(element_type == TagElement || element_type == UsedTagElement)
      throw SerializationError("Object cannot be stored in a property: " + fieldname);
    // First, check if this is a null pointer
    // A null pointer can be specified in one of two ways: a tag with the value "null"
    // or a single child with a name "null".  We only check for pointers,
    // because a lightweight object could easily have a value called "null".
    if(isPointer){
      xmlChar* prop = xmlGetProp(node, to_xml_ch_ptr(fieldname.c_str()));
      xmlNodePtr child = node->children;
      if((prop && strcmp(to_char_ptr(prop), "null") == 0)
         || (!prop && child && strcmp(to_char_ptr(child->name), "null") == 0)){
        if(!isPointer)
          throw SerializationError("Cannot read a reference to a null pointer");
        ptr.setNull();
        return;
      }
    }
    if(element_type == ContainerElement){
      // Second, see if this is a container - it must be handled separately
      // If this is a container element, try to create the next class
      if(!node || !current_container_child)
        throw SerializationError("Container must iterate reads with nextContainerObject");
      string classname = get_classname(current_container_child, fieldname);
      PointerWrapperInterface* newobj = 0;
      if(isPointer){
        if(!ptr.createObject(classname, &newobj))
          throw SerializationError("Cannot instantiate class: " + classname + " for field: " + fieldname);
      }
      const GenericRTTIInterface* classinfo = ptr.getRTTI();
      PersistentStorage::StorageHint storagehint = classinfo->storageHint();
      ElementType child_element_type = NormalElement;
      if(storagehint == PersistentStorage::Container)
        child_element_type = ContainerElement;

      // See if the object has a tag called "id".  If so, save that as the pointer name
      if(isPointer){
        xmlChar* id = xmlGetProp(current_container_child, to_xml_ch_ptr("id"));
        if(id){
          string sid(to_char_ptr(id));
          if(archive->pointermap.find(sid) != archive->pointermap.end())
            throw SerializationError("Pointer: " + sid + " defined twice while reading field: " + fieldname + " in class: " + classname);
          archive->pointermap.insert(std::make_pair(sid, newobj));
        }
      }

      XMLArchiveElement subelement(archive, current_container_child, child_element_type);
      ptr.readwrite(&subelement);
      return;
    }

    const GenericRTTIInterface* classinfo = ptr.getRTTI();
    PersistentStorage::StorageHint storagehint = classinfo->storageHint();

    // Third, look for a property with the fieldname
    xmlChar* prop = xmlGetProp(node, to_xml_ch_ptr(fieldname.c_str()));
    if(prop){
      // Make sure that there are no children with the same name
      for(xmlNode* child = node->children; child != 0; child = child->next){
        if(child->type == XML_ELEMENT_NODE && strcmp(to_char_ptr(child->name), fieldname.c_str()) == 0)
          throw SerializationError("Ambigious field: " + fieldname);
      }

      // If this is a lightweight object, read it directly.  Otherwise, it
      // should be the name of a pointer
      if(storagehint == PersistentStorage::Lightweight){
        XMLArchiveElement subelement(archive, node, fieldname);
        ptr.readwrite(&subelement);
        return;
      } else {
        string pointername = to_char_ptr(prop);
        pointermaptype::iterator iter = archive->pointermap.find(pointername);
        if(iter != archive->pointermap.end()){
          // Have pointer already
          if(!iter->second->upcast(&ptr))
            throw SerializationError("Pointer type mismatch while reading field: " + fieldname);
        } else {
          // Find pointer elsewhere in the file.  Usually this is either a forward
          // reference, or something in the Assets section
          assetmaptype::iterator iter = archive->assetmap.find(pointername);
          if(iter == archive->assetmap.end())
            throw SerializationError("Unknown pointer: " + pointername + " while reading field: " + fieldname);

          if(isPointer){
            string classname = get_classname(iter->second, fieldname);
            PointerWrapperInterface* newobj;
            if(!ptr.createObject(classname, &newobj))
              throw SerializationError("Cannot instantiate class: " + classname + " for field: " + fieldname);
            if(archive->pointermap.find(iter->first) != archive->pointermap.end())
              throw SerializationError("Pointer: " + iter->first + " defined twice while reading field: " + fieldname + " in class: " + classname);
            archive->pointermap.insert(std::make_pair(iter->first, newobj));
          }
          XMLArchiveElement subelement(archive, iter->second, NormalElement);
          ptr.readwrite(&subelement);
        }
        return;
      }
    }

    xmlNodePtr child = 0;
    ElementType child_element_type = NormalElement;
    PointerWrapperInterface* newobj = 0;
    string classname;

    // Fourth, look for a node with the fieldname
    child = node->children;
    while(child && (child->type != XML_ELEMENT_NODE || strcmp(to_char_ptr(child->name), fieldname.c_str()) != 0))
      child = child->next;
    if(child){
      // Make sure that there are no other children with the same name
      for(xmlNode* c = child->next; c != 0; c = c->next){
        if(c->type == XML_ELEMENT_NODE && strcmp(to_char_ptr(c->name), fieldname.c_str()) == 0)
          throw SerializationError("Ambiguous field: " + fieldname);
      }

      // The classname can be in one of several locations:
      // 1. As a property of the field with the tag "type"
      // 2. The sole child of the element
      // 3. Implied, but only if we are not reading a pointer or this is a container type
      xmlChar* type = xmlGetProp(child, to_xml_ch_ptr("type"));
      if(type){
        classname = to_char_ptr(type);
      } else if(storagehint == PersistentStorage::Container){
        classname = "<unknown>";
        child_element_type = ContainerElement;
      } else if(!isPointer){
        classname = "<fixed>";
      } else {
        xmlNode* c = child->children;
        while(c && c->type != XML_ELEMENT_NODE)
          c = c->next;
        if(c){
          child = c;
          classname = get_classname(c, fieldname);
        }
        c = c->next;
        while(c && c->type != XML_ELEMENT_NODE)
          c = c->next;
        if(c)
          throw SerializationError("Ambiguous value for field: " + fieldname + "(" + classname + " and " + to_char_ptr(c->name) + ")");
      }
    } else if(element_type == RootElement){
      // If this is the root element, find the first class that matches
      child = node->children;
      do {
        while(child && child->type != XML_ELEMENT_NODE)
          child = child->next;
        if(child){
          classname = get_classname(child, fieldname);
          if(!ptr.createObject(classname, &newobj))
            child = child->next;
        } else {
          throw SerializationError("Cannot find root object for field: " + fieldname);
        }
      } while(child && !newobj);
    } else {
      throw SerializationError("Cannot find field: " + fieldname);
    }

    if(isPointer && !newobj){
      if(!ptr.createObject(classname, &newobj))
        throw SerializationError("Cannot instantiate class: " + classname + " for field: " + fieldname);
    }

    // See if the object has a tag called "id".  If so, save that as the pointer name
    xmlChar* id = xmlGetProp(child, to_xml_ch_ptr("id"));
    if(id){
      string sid(to_char_ptr(id));
      if(archive->pointermap.find(sid) != archive->pointermap.end())
        throw SerializationError("Pointer: " + sid + " defined twice while reading field: " + fieldname + " in class: " + classname);
      archive->pointermap.insert(std::make_pair(sid, newobj));
    }
    XMLArchiveElement subelement(archive, child, child_element_type);
    ptr.readwrite(&subelement);
  } else {
    // Writing
    if(element_type == TagElement || element_type == UsedTagElement)
      throw SerializationError("Creating complex object in lightweight archive element");
    if(ptr.isNull()){
      // A null pointer, mark it appropriately
      xmlSetProp(node, to_xml_ch_ptr(fieldname.c_str()), to_xml_ch_ptr("null"));
      return;
    }
    // Check to see if this pointer has already been emitted.
    if(isPointer){
      void* id = ptr.getUniqueID();
      refmaptype::iterator iter = archive->refmap.find(id);
      if(iter != archive->refmap.end()){
        // This is a reference to a pointer that has already been
        // emitted.  Emit the reference.
        RefInfo& ri = iter->second;
        if(ri.useCount == 1){
          // Append the count to make it unique
          // We do it here so that we only change the number for pointers
          // that are actually used more than once
          countmaptype::iterator citer = archive->countmap.find(ri.refname);
          int count;
          if(citer == archive->countmap.end()){
            count = 0;
            archive->countmap.insert(make_pair(ri.refname, 1));
          } else {
            count = citer->second++;
          }

          ostringstream str;
          str << ri.refname << count;
          ri.refname = str.str();
          xmlSetProp(ri.node, to_xml_ch_ptr("id"), to_xml_ch_ptr(ri.refname.c_str()));
        }
        ri.useCount++;
        if(xmlHasProp(node, to_xml_ch_ptr(fieldname.c_str())))
          throw SerializationError("Pointer written twice");
        xmlSetProp(node, to_xml_ch_ptr(fieldname.c_str()), to_xml_ch_ptr(ri.refname.c_str()));
        return;
      }
    }
    const GenericRTTIInterface* classinfo = ptr.getRTTI();
    bool have_complex_classname;
    string safe_classname = classinfo->getPublicClassname();
    make_safe_classname(safe_classname, have_complex_classname);

    PersistentStorage::StorageHint storagehint = classinfo->storageHint();
    if(storagehint == PersistentStorage::Lightweight && !isPointer){
      // Store one value in the tag
      XMLArchiveElement subelement(archive, node, fieldname);
      ptr.readwrite(&subelement);
    } else {
      xmlNodePtr childnode;
      ElementType child_element_type = NormalElement;
      if(element_type == RootElement || element_type == ContainerElement){
        // Store only the class name
        childnode = xmlNewNode(0, to_xml_ch_ptr(safe_classname.c_str()));
        if(have_complex_classname)
          xmlSetProp(childnode, to_xml_ch_ptr("type"), to_xml_ch_ptr(safe_classname.c_str()));
        xmlAddChild(node, childnode);
      } else if(storagehint == PersistentStorage::Container){
        // Store only the field name
        childnode = xmlNewNode(0, to_xml_ch_ptr(fieldname.c_str()));
        xmlAddChild(node, childnode);
        child_element_type = ContainerElement;
      } else if(!isPointer){
        // Store only the field name
        childnode = xmlNewNode(0, to_xml_ch_ptr(fieldname.c_str()));
        xmlAddChild(node, childnode);
      } else {
        // Store both field name and class
        childnode = xmlNewNode(0, to_xml_ch_ptr(safe_classname.c_str()));
        if(have_complex_classname)
          xmlSetProp(childnode, to_xml_ch_ptr("type"), to_xml_ch_ptr(safe_classname.c_str()));
        xmlNodePtr fieldnode = xmlNewNode(0, to_xml_ch_ptr(fieldname.c_str()));
        xmlAddChild(fieldnode, childnode);
        xmlAddChild(node, fieldnode);
      }

      if(isPointer){
        // Save this pointer for possible later use
        RefInfo ri;
        ri.node = childnode;
        ri.refname = classinfo->getPublicClassname(); // Will be appended with unique ID in haveReference
        ri.useCount = 1;
        void* id = ptr.getUniqueID();
        archive->refmap.insert(make_pair(id, ri));
      }
      XMLArchiveElement subelement(archive, childnode, child_element_type);
      ptr.readwrite(&subelement);
    }
  }
}
