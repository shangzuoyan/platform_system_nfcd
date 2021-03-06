/*
 * Copyright (C) 2013-2014  Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "RouteDataSet.h"
#include <expat.h>
#include <stdio.h>

#include "NfcDebug.h"

extern char bcm_nfc_location[];

const uint32_t BUF_SIZE = 1024;
const char* RouteDataSet::sConfigFile = "/param/route.xml";

/**
 * Route.xml tag, property and value definition.
 */
static const char* TAG_ROUTE = "Route";
static const char* TAG_TECH = "Tech";
static const char* TAG_PROTO = "Proto";

static const char* PROP_TYPE = "Type";
static const char* PROP_ID = "Id";
static const char* PROP_SECELEM = "SecElem";
static const char* PROP_SWITCH_ON = "SwitchOn";
static const char* PROP_SWITCH_OFF = "SwitchOff";
static const char* PROP_BATTERY_OFF = "BatteryOff";

static const char* PROTO_ID_T1T = "T1T";
static const char* PROTO_ID_T2T = "T2T";
static const char* PROTO_ID_T3T = "T3T";
static const char* PROTO_ID_ISO_DEP = "IsoDep";

static const char* TECH_ID_NFCA = "NfcA";
static const char* TECH_ID_NFCB = "NfcB";
static const char* TECH_ID_NFCF = "NfcF";

static const char* ROUTE_TYPE_DEFAULT = "DefaultRoutes";
static const char* ROUTE_TYPE_SEC = "SecElemSelectedRoutes";

static const char* BOOL_TRUE = "true";
static const char* BOOL_FALSE = "false";

void RouteDataSet::ImportProtocolRoute(const char** aAttribute)
{
  RouteDataForProtocol* data = new RouteDataForProtocol;

  for (int i = 0; aAttribute[i]; i += 2) {
    const char* prop = aAttribute[i];
    const char* value = aAttribute[i + 1];

    if (strcmp(prop, PROP_ID) == 0) {
      if (strcmp(value, PROTO_ID_T1T) == 0) {
        data->mProtocol = NFA_PROTOCOL_MASK_T1T;
      } else if (strcmp(value, PROTO_ID_T2T) == 0) {
        data->mProtocol = NFA_PROTOCOL_MASK_T2T;
      } else if (strcmp(value, PROTO_ID_T3T) == 0) {
        data->mProtocol = NFA_PROTOCOL_MASK_T3T;
      } else if (strcmp(value, PROTO_ID_ISO_DEP) == 0) {
        data->mProtocol = NFA_PROTOCOL_MASK_ISO_DEP;
      }
    } else if (strcmp(prop, PROP_SECELEM) == 0) {
      data->mNfaEeHandle = strtol((char*)value, NULL, 16);
      data->mNfaEeHandle = data->mNfaEeHandle | NFA_HANDLE_GROUP_EE;
    } else if (strcmp(prop, PROP_SWITCH_ON) == 0) {
      data->mSwitchOn = (strcmp(value, BOOL_TRUE) == 0);
    } else if (strcmp(prop, PROP_SWITCH_OFF) == 0) {
      data->mSwitchOff = (strcmp(value, BOOL_TRUE) == 0);
    } else if (strcmp(prop, PROP_BATTERY_OFF) == 0) {
      data->mBatteryOff = (strcmp(value, BOOL_TRUE) == 0);
    }
  }
  mCurrentDB->push_back(data);
}

void RouteDataSet::ImportTechnologyRoute(const char** aAttribute)
{
  RouteDataForTechnology* data = new RouteDataForTechnology;

  for (int i = 0; aAttribute[i]; i += 2) {
    const char* prop = aAttribute[i];
    const char* value = aAttribute[i + 1];

    if (strcmp(prop, PROP_ID) == 0) {
      if (strcmp(value, TECH_ID_NFCA) == 0) {
        data->mTechnology = NFA_TECHNOLOGY_MASK_A;
      } else if (strcmp(value, TECH_ID_NFCB) == 0) {
        data->mTechnology = NFA_TECHNOLOGY_MASK_B;
      } else if (strcmp(value, TECH_ID_NFCF) == 0) {
        data->mTechnology = NFA_TECHNOLOGY_MASK_F;
      }
    } else if (strcmp(prop, PROP_SECELEM) == 0) {
      data->mNfaEeHandle = strtol((char*)value, NULL, 16);
      data->mNfaEeHandle = data->mNfaEeHandle | NFA_HANDLE_GROUP_EE;
    } else if (strcmp(prop, PROP_SWITCH_ON) == 0) {
      data->mSwitchOn = (strcmp(value, BOOL_TRUE) == 0);
    } else if (strcmp(prop, PROP_SWITCH_OFF) == 0) {
      data->mSwitchOff = (strcmp(value, BOOL_TRUE) == 0);
    } else if (strcmp(prop, PROP_BATTERY_OFF) == 0) {
      data->mBatteryOff = (strcmp(value, BOOL_TRUE) == 0);
    }
  }
  mCurrentDB->push_back(data);
}

void RouteDataSet::XmlStartElement(void* aData,
                                   const char* aElement,
                                   const char** aAttribute)
{
  RouteDataSet* route = reinterpret_cast<RouteDataSet*>(aData);
  if (!route) {
    return;
  }

  if (strcmp(aElement, TAG_ROUTE) == 0) {
    for (uint32_t i = 0; aAttribute[i]; i += 2) {
      const char* prop = aAttribute[i];
      const char* value = aAttribute[i + 1];

      if (strcmp(prop, PROP_TYPE) == 0) {
        if (strcmp(value, ROUTE_TYPE_DEFAULT) == 0) {
          route->mCurrentDB = &(route->mDefaultRouteDatabase);
        } else if (strcmp(value, ROUTE_TYPE_SEC) == 0) {
          route->mCurrentDB = &(route->mSecElemRouteDatabase);
        }
      }
    }
  } else if (strcmp(aElement, TAG_TECH) == 0) {
    route->ImportTechnologyRoute(aAttribute);
  } else if (strcmp(aElement, TAG_PROTO) == 0) {
    route->ImportProtocolRoute(aAttribute);
  }
}

void RouteDataSet::XmlEndElement(void* aData,
                                 const char* aElement)
{
}

RouteDataSet::~RouteDataSet()
{
  DeleteDatabase();
}

bool RouteDataSet::Initialize()
{
  return true;
}

void RouteDataSet::DeleteDatabase()
{
  NCI_DEBUG("default db size=%u; sec elem db size=%u",
            mDefaultRouteDatabase.size(), mSecElemRouteDatabase.size());
  Database::iterator it;

  for (it = mDefaultRouteDatabase.begin(); it != mDefaultRouteDatabase.end(); it++) {
    delete (*it);
  }
  mDefaultRouteDatabase.clear();

  for (it = mSecElemRouteDatabase.begin(); it != mSecElemRouteDatabase.end(); it++) {
    delete (*it);
  }
  mSecElemRouteDatabase.clear();
}

bool RouteDataSet::Import()
{
  NCI_DEBUG("enter");

  std::string strFilename(bcm_nfc_location);
  strFilename += sConfigFile;

  DeleteDatabase();

  FILE* file = fopen(strFilename.c_str(), "r");
  if (!file) {
    NCI_DEBUG("Failed to open %s", strFilename.c_str());
    return false;
  }

  XML_Parser parser = XML_ParserCreate(NULL);
  if (!parser) {
    NCI_ERROR("Failed to create XML parser");
    return false;
  }

  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser,
                        RouteDataSet::XmlStartElement,
                        RouteDataSet::XmlEndElement);

  while (true) {
    size_t actual = 0;
    void* buffer = XML_GetBuffer(parser, BUF_SIZE);
    if (buffer == NULL) {
      break;
    }

    actual = fread(buffer, sizeof(char), BUF_SIZE, file);
    if (actual == 0) {
      break;
    }

    if (XML_ParseBuffer(parser, actual, actual == 0) == XML_STATUS_ERROR) {
      break;
    }
  }

  XML_ParserFree(parser);
  fclose(file);

  return true;
}
