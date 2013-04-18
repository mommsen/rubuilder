#include "interface/evb/i2oEVBMsgs.h"

#include <iostream>
#include <stdint.h>


int main(int argv, char **argc)
{
  std::cout << "=====================================" << std::endl;
  std::cout << "= RU builder message sizes in bytes =" << std::endl;
  std::cout << "=====================================" << std::endl;

  std::cout << std::endl;

  std::cout << "I2O_MESSAGE_FRAME                  = ";
  std::cout << sizeof(I2O_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "I2O_PRIVATE_MESSAGE_FRAME          = ";
  std::cout << sizeof(I2O_PRIVATE_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME = ";
  std::cout << sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "BU_ALLOCATE                        = ";
  std::cout << sizeof(BU_ALLOCATE);
  std::cout << std::endl;

  std::cout << "I2O_BU_ALLOCATE_MESSAGE_FRAME      = ";
  std::cout << sizeof(I2O_BU_ALLOCATE_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "I2O_BU_COLLECT_MESSAGE_FRAME       = ";
  std::cout << sizeof(I2O_BU_COLLECT_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "I2O_BU_DISCARD_MESSAGE_FRAME       = ";
  std::cout << sizeof(I2O_BU_DISCARD_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "I2O_TA_CREDIT_MESSAGE_FRAME        = ";
  std::cout << sizeof(I2O_TA_CREDIT_MESSAGE_FRAME);
  std::cout << std::endl;

  std::cout << "EvtIdRqstAndOrRelease              = ";
  std::cout << sizeof(EvtIdRqstAndOrRelease);
  std::cout << std::endl;

  std::cout << "EvtIdRqstsAndOrReleasesMsg         = ";
  std::cout << sizeof(EvtIdRqstsAndOrReleasesMsg);
  std::cout << std::endl;

  std::cout << "EvtIdTrigPair                      = ";
  std::cout << sizeof(EvtIdTrigPair);
  std::cout << std::endl;

  std::cout << "EvtIdTrigPairsMsg                  = ";
  std::cout << sizeof(EvtIdTrigPairsMsg);
  std::cout << std::endl;

  std::cout << "RqstForFrag                        = ";
  std::cout << sizeof(RqstForFrag);
  std::cout << std::endl;

  std::cout << "RqstForFragsMsg                    = ";
  std::cout << sizeof(RqstForFragsMsg);
  std::cout << std::endl;

  return 0;
}
