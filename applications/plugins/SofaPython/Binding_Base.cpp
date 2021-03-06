/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2018 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/

#include "Binding_Base.h"
#include "Binding_Data.h"
#include "Binding_Link.h"

#include <sofa/helper/vector.h>
#include <sofa/core/DataEngine.h>
#include <sofa/defaulttype/Vec3Types.h>


#include <sofa/helper/logging/Messaging.h>

#include "PythonFactory.h"
#include "PythonToSofa.inl"

#include "PythonEnvironment.h"
using sofa::simulation::PythonEnvironment ;
using sofa::core::topology::BaseMeshTopology ;
typedef BaseMeshTopology::Tetra Tetra;
using sofa::helper::vector;
using sofa::helper::Factory;
using namespace sofa::core::objectmodel;

// TODO (sescaida 13.02.2018): this factory code is redundant to the Communication plugin, but should easily be mergeable, when an adequate spot is found.
typedef sofa::helper::Factory< std::string, BaseData> PSDEDataFactory;

PSDEDataFactory* getFactoryInstance(){
    static PSDEDataFactory* s_localfactory = nullptr ;
    if(s_localfactory==nullptr)
    {
        s_localfactory = new PSDEDataFactory() ;
        s_localfactory->registerCreator("s", new DataCreator<std::string>());
        s_localfactory->registerCreator("f", new DataCreator<float>());
        s_localfactory->registerCreator("b", new DataCreator<bool>());
        s_localfactory->registerCreator("d", new DataCreator<int>());
        s_localfactory->registerCreator("t", new DataCreator<vector<Tetra>>());
        s_localfactory->registerCreator("p", new DataCreator<sofa::defaulttype::Vec3dTypes::VecCoord>());
    }
    return s_localfactory ;
}


static Base* get_base(PyObject* self) {
    return sofa::py::unwrap<Base>(self);
}

static char* getStringCopy(char *c)
{
    if (c==nullptr)
        return nullptr;

    char* tmp = new char[strlen(c)+1] ;
    strcpy(tmp,c);
    return tmp ;
}


// helper function for parsing Python arguments
// not defined static in order to be able to use this fcn somewhere else also
BaseData* helper_addNewData(PyObject *args, PyObject * kw, Base * obj) {

    char* dataRawType = new char;
    char* dataClass = new char;
    char* dataHelp = new char;
    char * dataName = new char;

    PyObject* dataValue = nullptr;

    bool KwargsOrArgs = 0; //Args = 0, Kwargs = 1

    if(PyArg_ParseTuple(args, "s|sssO", &dataName, &dataClass, &dataHelp, &dataRawType, &dataValue))
    {
        // first argument (name) is mandatory, the rest are optionally found in the args and, if not there, in kwargs
        dataName = getStringCopy(dataName) ;

        if (strcmp(dataName,"")==0)
        {
            return nullptr;
        }

        if (dataValue==nullptr) // the content of dataValue is not set, so parsing normal args didn't succeed fully --> look for kwargs
        {
            KwargsOrArgs = 1;
        }
        else // arguments are available ...
        {
            dataClass = getStringCopy(dataClass) ;
            dataHelp  = getStringCopy(dataHelp) ;
            dataRawType  = getStringCopy(dataRawType) ;
            Py_IncRef(dataValue); // want to hold on it for a while
        }
    }
    else
    {
        return nullptr;
    }    

    if(KwargsOrArgs) // parse kwargs
    {
        if(kw==nullptr || !PyDict_Check(kw) )
        {
            msg_error("SofaPython") << "Could not parse kwargs for adding Data";
            return nullptr;
        }
        PyObject * tmp;
        tmp = PyDict_GetItemString(kw,"datatype");
        if (tmp!=nullptr){
            dataRawType = getStringCopy(PyString_AsString(tmp));
        }

        tmp = PyDict_GetItemString(kw,"helptxt");
        if (tmp!=nullptr){
            dataHelp = getStringCopy(PyString_AsString(tmp));
        }

        tmp = PyDict_GetItemString(kw,"dataclass");
        if (tmp!=nullptr){
            dataClass= getStringCopy(PyString_AsString(tmp));
        }

        tmp = PyDict_GetItemString(kw,"value");
        if (tmp!=nullptr){
            dataValue = tmp;
            Py_IncRef(dataValue); // call to Py_GetItemString doesn't increment the ref count, but we want to hold on to it for a while ...
        }      
    }

    if (dataRawType[0]==0) // We cannot construct without a type
    {
        msg_error(obj) << "No type provided for Data, cannot construct/add";
        return nullptr;
    }

    BaseData* bd = getFactoryInstance()->createObject(dataRawType, sofa::helper::NoArgument());

    if (bd == nullptr)
    {
        msg_error(obj) << dataRawType << " is not a known type";
        return nullptr;
    }
    else
    {
        bd->setName(dataName);
        bd->setHelp(dataHelp);
        obj->addData(bd);        
        if(dataValue!=nullptr) // parse provided data: Py->SofaStr->Data or link
        {
            std::stringstream tmp;
            pythonToSofaDataString(dataValue, tmp);
            if(tmp.str()[0]=='@' && bd->canBeLinked())
            {
                if(!bd->setParent(tmp.str()))
                {
                    msg_warning(obj) << "Could not setup link for Data, initialzing empty";
                }
            }
            else
            {
                bd->read( tmp.str() );
            }
            bd->setGroup(dataClass);
            Py_DecRef(dataValue);
        }
    }
    return bd;
}

static PyObject * Base_addData(PyObject *self, PyObject *args )
{
    Base* obj = get_base(self);
    PyObject* pydata;

    if (!PyArg_ParseTuple(args, "O", &pydata)) {
        return NULL;
    }

    BaseData* data = sofa::py::unwrap<BaseData>(pydata);
    obj->addData(data) ;

    Py_RETURN_NONE;
}


static PyObject * Base_addNewData(PyObject *self, PyObject *args) {
    Base* obj = get_base(self);
    if( helper_addNewData(args, nullptr, obj) == nullptr )
        return nullptr ;
    Py_RETURN_NONE;
}

static PyObject * Base_getData(PyObject *self, PyObject *args ) {
    Base* obj = get_base(self);
    char *dataName;
    if (!PyArg_ParseTuple(args, "s", &dataName)) {
        return NULL;
    }

    BaseData * data = obj->findData(dataName);
    if (!data)
    {
        Py_RETURN_NONE ;
    }

    return SP_BUILD_PYPTR(Data,BaseData,data,false);
}

static PyObject * Base_getLink(PyObject *self, PyObject *args ) {
    Base* obj = get_base(self);
    char *dataName;
    if (!PyArg_ParseTuple(args, "s", &dataName)) {
        return NULL;
    }

    BaseLink *link = obj->findLink(dataName);
    if (!link)
    {
        Py_RETURN_NONE ;
    }

    return SP_BUILD_PYPTR(Link,BaseLink,link,false);
}

static PyObject * Base_findData(PyObject *self, PyObject *args ) {
    Base* obj = get_base(self);
    char *dataName;

    if (!PyArg_ParseTuple(args, "s", &dataName)) {
        return NULL;
    }

    BaseData * data = obj->findData(dataName);
    if (!data) {
        std::stringstream tmp ;
        if( obj->hasField(dataName) ) {
            tmp <<"object '"<<obj->getName()<<"' has a field '"<<dataName<<"' but it is not a Data";
        } else {
            tmp << "object '"<<obj->getName()<<"' does no have a field '"<<dataName<<"'";
            obj->writeDatas(tmp,";");
        }

        PyErr_SetString(PyExc_RuntimeError, tmp.str().c_str());
        return NULL;
    }

    /// special cases... from factory (e.g DisplayFlags, OptionsGroup)
    {
        PyObject* res = sofa::PythonFactory::toPython(data);
        if( res ) return res;
    }

    return SP_BUILD_PYPTR(Data,BaseData,data,false);
}


static PyObject * Base_findLink(PyObject *self, PyObject *args) {
    Base* obj = get_base(self);
    char *linkName;
    if (!PyArg_ParseTuple(args, "s", &linkName)) {
        return NULL;
    }

    BaseLink * link = obj->findLink(linkName);
    if (!link) {
        std::stringstream tmp ;
        if( obj->hasField(linkName) ) {
            tmp << "object '"<<obj->getName()<<"' has a field '"<<linkName<<"' but it is not a Link";
        } else {
            tmp <<"object '"<<obj->getName()<<"' does no have a field '"<<linkName<<"'" << msgendl;
            obj->writeDatas(tmp,";");
        }

        PyErr_SetString(PyExc_RuntimeError, tmp.str().c_str());
        return NULL;
    }

    return SP_BUILD_PYPTR(Link,BaseLink,link,false);
}

/// Generic accessor to Data fields (in python native type)
static PyObject* Base_GetAttr(PyObject *o, PyObject *attr_name) {
    Base* obj = get_base(o);
    char *attrName = PyString_AsString(attr_name);

    /// see if a Data field has this name...
    if( BaseData * data = obj->findData(attrName) ) {
        /// special cases... from factory (e.g DisplayFlags, OptionsGroup)
        if( PyObject* res = sofa::PythonFactory::toPython(data) ) {
            return res;
        } else {
            /// the data type is not known by the factory, let's create the right Python type....
            return GetDataValuePython(data);
        }
    }

    /// see if a Link has this name...
    if( BaseLink * link = obj->findLink(attrName) ) {
        /// we have our link... let's create the right Python type....
        return GetLinkValuePython(link);
    }

    return PyObject_GenericGetAttr(o,attr_name);
}

static int Base_SetAttr(PyObject *o, PyObject *attr_name, PyObject *v) {
    /// attribute does not exist: see if a Data field has this name...
    Base* obj = get_base(o);
    char *attrName = PyString_AsString(attr_name);

    if (BaseData * data = obj->findData(attrName)) {
        /// data types in Factory can have a specific setter
        if( PyObject* pyData = sofa::PythonFactory::toPython(data) ) {
            return PyObject_SetAttrString( pyData, "value", v );
        } else {
            /// the data type is not known by the factory, let's use the default implementation
            return SetDataValuePython(data,v);
        }
    }

    if (BaseLink * link = obj->findLink(attrName)) {
        return SetLinkValuePython(link,v);
    }

    return PyObject_GenericSetAttr(o,attr_name,v);
}

static PyObject * Base_getClassName(PyObject * self, PyObject * /*args*/) {
    /// BaseNode is not bound in SofaPython, so getPathName is bound in Node instead
    Base* node = get_base(self);

    return PyString_FromString(node->getClassName().c_str());
}

static PyObject * Base_getTemplateName(PyObject * self, PyObject * /*args*/) {
    /// BaseNode is not bound in SofaPython, so getPathName is bound in Node instead
    Base* node = get_base(self);

    return PyString_FromString(node->getTemplateName().c_str());
}

static PyObject * Base_getName(PyObject * self, PyObject * /*args*/) {
    /// BaseNode is not bound in SofaPython, so getPathName is bound in Node instead
    Base* node = get_base(self);

    return PyString_FromString(node->getName().c_str());
}

static PyObject * Base_getDataFields(PyObject *self, PyObject * /*args*/) {
    Base * component = get_base(self);

    const sofa::helper::vector<BaseData*> dataFields = component->getDataFields();

    PyObject * pyDict = PyDict_New();
    for (size_t i=0; i<dataFields.size(); i++) {
        PyDict_SetItem(pyDict, PyString_FromString(dataFields[i]->getName().c_str()),
                       GetDataValuePython(dataFields[i]));
    }

    return pyDict;
}


/// This function is named this way because someone give the getDataFields to the one
/// that returns a dictionary of (name, value) which is not coherente with the c++
/// name of the function.
/// If you are a brave hacker, courageous enough to break backward compatibility you can
/// probably fix all this mess.
static PyObject * Base_getListOfDataFields(PyObject *self, PyObject * /*args*/) {
    Base * component = get_base(self);

    const sofa::helper::vector<BaseData*> dataFields = component->getDataFields();

    PyObject * pyList = PyList_New(dataFields.size());
    for (unsigned int i = 0; i < dataFields.size(); ++i) {
        PyList_SetItem(pyList, i, SP_BUILD_PYPTR(Data,BaseData,dataFields[i],false)) ;
    }

    return pyList;
}

static PyObject * Base_getListOfLinks(PyObject *self, PyObject * /*args*/) {
    Base * component = get_base(self);

    const sofa::helper::vector<BaseLink*> links = component->getLinks() ;

    PyObject * pyList = PyList_New(links.size());
    for (unsigned int i = 0; i < links.size(); ++i) {
        PyList_SetItem(pyList, i, SP_BUILD_PYPTR(Link, BaseLink, links[i], false)) ;
    }

    return pyList;
}

/// down cast to the lowest type known by the factory
/// there is maybe a more pythonish way to do so? :)
static PyObject * Base_downCast(PyObject *self, PyObject * /*args*/) {
    Base* component = get_base(self);
    return sofa::PythonFactory::toPython(component);
}

SP_CLASS_METHODS_BEGIN(Base)
SP_CLASS_METHOD_DOC(Base,addNewData, "Add a new Data field to the current object. \n"
                                        "Eg:                                         \n"
                                        "  obj.addNewData('myDataName1','theDataGroupA','help message','f',1.0)  \n"
                                        "  obj.addNewData('myDataName2','theDataGroupA','help message','b',True) \n"
                                        "  obj.addNewData('myDataName3','theDataGroupB','help message','d',1)     \n"
                                        "  obj.addNewData('myDataName4','theDataGroupB','help message','s','hello') \n")
SP_CLASS_METHOD_DOC(Base,addData, "Adds an existing data field to the current object")
SP_CLASS_METHOD_DOC(Base,findData, "Returns the data field if there is one associated \n"
                                   "with the provided name and downcasts it to the lowest known type. \n"
                                   "Returns None otherwhise.")
SP_CLASS_METHOD_DOC(Base,findLink, "Returns a link field if there is one associated \n"
                                   "with the provided name, returns None otherwhise")
SP_CLASS_METHOD_DOC(Base,getData, "Returns the data field if there is one associated \n"
                              "with the provided name but don't downcasts it to the lowest known type. \n"
                              "Returns None is there is no field with this name.")
SP_CLASS_METHOD_DOC(Base,getLink, "Returns the link field if there is one associated \n"
                              "with the provided name but. \n"
                              "Returns None is there is no field with this name.")
SP_CLASS_METHOD(Base,getClassName)
SP_CLASS_METHOD(Base,getTemplateName)
SP_CLASS_METHOD(Base,getName)

SP_CLASS_METHOD_DOC(Base,getDataFields, "Returns a list with the *content* of all the data fields converted in python"
                                        " type. \n")
SP_CLASS_METHOD_DOC(Base,getListOfDataFields, "Returns the list of data fields.")
SP_CLASS_METHOD_DOC(Base,getListOfLinks, "Returns the list of link fields.")
SP_CLASS_METHOD(Base,downCast)
SP_CLASS_METHODS_END;


SP_CLASS_ATTRS_BEGIN(Base)
SP_CLASS_ATTRS_END;

SP_CLASS_TYPE_BASE_SPTR_ATTR_GETATTR(Base, Base)
