/**********************************************************************
    The following code is derived, directly or indirectly, from the SystemC
    source code Copyright (c) 1996-2008 by all Contributors.
    All Rights reserved.
 
    The contents of this file are subject to the restrictions and limitations
    set forth in the SystemC Open Source License Version 3.0 (the "License");
    You may not use this file except in compliance with such restrictions and
    limitations. You may obtain instructions on how to receive a copy of the
    License at http://www.systemc.org/. Software distributed by Contributors
    under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
    ANY KIND, either express or implied. See the License for the specific
    language governing rights and limitations under the License.
 *********************************************************************/

//=====================================================================
/// @file traffic_generator.cpp
// 
/// @brief traffic generation routines
//
//=====================================================================
//  Authors:
//    Bill Bunton, ESLX
//    Jack Donovan, ESLX
//    Charles Wilson, ESLX
//====================================================================

#include "reporting.h"               	// reporting macros
#include "traffic_generator.h"          // traffic generator declarations

using namespace std;

static const char *filename = "traffic_generator.cpp";  ///< filename for reporting

/// Constructor

SC_HAS_PROCESS(traffic_generator);
//-----------------------------------------------------------------------------
//
traffic_generator::traffic_generator            // constructor
( sc_core::sc_module_name name                  // instance name
, const unsigned int    ID                      // initiator ID
, sc_dt::uint64         base_address_1          // first base address
, sc_dt::uint64         base_address_2          // second base address
)
: sc_module           ( name            )       /// module name
, m_ID                ( ID              )       /// initiator ID
, m_base_address_1    ( base_address_1  )       /// first base address
, m_base_address_2    ( base_address_2  )       /// second base address
, m_active_txn_count  ( 4               )       /// Max number of transactions active 
, m_check_all         ( false           )       /// @todo comment variable
{ 
  SC_THREAD(traffic_generator_thread);
}
//-----------------------------------------------------------------------------
//
traffic_generator::traffic_generator            // @todo keep me, lose other constructor
( sc_core::sc_module_name name                  // module name
, const unsigned int    ID                      // initiator ID
, sc_dt::uint64         base_address_1          // first base address
, sc_dt::uint64         base_address_2          // second base address
, unsigned int          active_txn_count        // Max number of active transactions 
)

: sc_module           ( name              )     /// module name
, m_ID                ( ID                )     /// initiator ID
, m_base_address_1    ( base_address_1    )     /// first base address
, m_base_address_2    ( base_address_2    )     /// second base address
, m_active_txn_count  ( active_txn_count  )     /// Max number of active transactions 
, m_check_all         ( false             )
{ 
  SC_THREAD(traffic_generator_thread);
}

/// SystemC thread for generation of GP traffic

void
traffic_generator::traffic_generator_thread   
( void
 )
{
  std::ostringstream  msg;                      ///< log message
  msg.str ("");
  msg << "Initiator: " << m_ID << " Starting Traffic";
  REPORT_INFO(filename, __FUNCTION__, msg.str());
  
  tlm::tlm_generic_payload  *transaction_ptr;
  unsigned char             *data_buffer_ptr; 
  
  // build transaction pool 

  for (unsigned int i = 0; i < m_active_txn_count; i++ )
  {
    m_txn_pool.push ();
    
//    data_buffer_ptr  = new unsigned char [m_txn_data_size];
//    transaction_ptr  = new tlm::tlm_generic_payload;
    
//    transaction_ptr->set_data_ptr(data_buffer_ptr);
    
//    m_txn_pool.push(transaction_ptr);
  }
  
  // outer loop of a simple memory test  Generate addresses

  sc_dt::uint64 base_address;
   
  for (unsigned int i = 0; i < 2; i++ ) {
    if (i==0) {
      base_address  = m_base_address_1; 
    }
    else {
      base_address  = m_base_address_2; 
    }

    sc_dt::uint64 mem_address = base_address; 

  //-----------------------------------------------------------------------------
  // write loop 
    for (unsigned int j = 0; j < 16; j++ ) {
    
      if(!m_txn_pool.empty())
      {
//        transaction_ptr = m_txn_pool.front();   // get gp from pool 
//        m_txn_pool.pop();
        
        transaction_ptr = m_txn_pool.pop ();
        
        data_buffer_ptr = transaction_ptr->get_data_ptr();

        unsigned int w_data = (unsigned int)mem_address;
        
        // invert data on second pass
        if (i==1) {
          w_data = ~w_data;
          }
      
        // convert address of write data to an 32-bit value
        *reinterpret_cast<unsigned int*>(data_buffer_ptr) = w_data;
        
        transaction_ptr->set_command          ( tlm::TLM_WRITE_COMMAND       );
        transaction_ptr->set_address          ( mem_address                  );
        transaction_ptr->set_data_length      ( m_txn_data_size              );
        transaction_ptr->set_response_status  ( tlm::TLM_INCOMPLETE_RESPONSE );

        mem_address += m_txn_data_size;             // increment memory address 

        request_out_port->write (transaction_ptr);  // send write request 
      } 
      check_complete();
          
    } // end write loop
    
    check_all_complete();
    
    // read loop 
  
    mem_address = base_address; 
   
    for (unsigned int i = 0; i < 16; i++ )
    {
      if(!m_txn_pool.empty())
      {
//        transaction_ptr = m_txn_pool.front();   // get gp from pool 
//        m_txn_pool.pop();
        
        transaction_ptr = m_txn_pool.pop ();
                           
        transaction_ptr->set_command          ( tlm::TLM_READ_COMMAND        );
        transaction_ptr->set_address          ( mem_address                  );
        transaction_ptr->set_data_length      ( m_txn_data_size              );
        transaction_ptr->set_response_status  ( tlm::TLM_INCOMPLETE_RESPONSE );

        mem_address += m_txn_data_size;             // increment memory address 

        request_out_port->write (transaction_ptr);  // send write request 
      }
        
      check_complete();
    } // end read loop
   
    check_all_complete();
  }

  msg.str ("");
  msg << "Traffic Generator : " << m_ID << endl 
  << "=========================================================" << endl 
  << "            ####  Traffic Generator Complete  #### ";
  REPORT_INFO(filename, __FUNCTION__, msg.str());

} // end traffic_generator_thread

//-----------------------------------------------------------------------------
//  Check Complete method

void traffic_generator::check_complete (void) {
  std::ostringstream        msg;   
  tlm::tlm_generic_payload  *transaction_ptr;
    
  if (  m_txn_pool.empty() 
      || m_check_all 
      ||(response_in_port->num_available() > 0)
      )
      {
    response_in_port->read(transaction_ptr);
    
    if (transaction_ptr ->get_response_status() != tlm::TLM_OK_RESPONSE) {
      msg.str ("");
      msg << m_ID << "Transaction ERROR";
    REPORT_FATAL(filename, __FUNCTION__, msg.str()); 
    }
    
    if (  transaction_ptr->get_command() == tlm::TLM_READ_COMMAND){
      unsigned int    expected_data   = (unsigned int)transaction_ptr->get_address();
      unsigned char*  data_buffer_ptr = transaction_ptr->get_data_ptr();
      unsigned int    read_data       = *reinterpret_cast<unsigned int*>(data_buffer_ptr);
    
//-----------------------------------------------------------------------------
// The address for the �gp� is used as expected data.  The address filed of 
//  the �gp� is a mutable field and is changed by the SimpleBus interconnect. 
//  The list significant 28 bits are not modified and are use for comparison.    
// @todo make sure that the documentation makes the volatility of GP address   
      unsigned int read_data_msk =  read_data & 0x0fffffff;
    
      if (!(    read_data_msk == ( expected_data & 0x0fffffff)
            ||  read_data_msk == (~expected_data & 0x0fffffff)
            ) ) {
        msg.str ("");
        msg << m_ID << " Memory read data ERROR";
        REPORT_FATAL(filename, __FUNCTION__, msg.str()); 
        }
      }
    m_txn_pool.push(transaction_ptr);
    }
  } // end check_complete

//-----------------------------------------------------------------------------
//  Check All Complete method

void traffic_generator::check_all_complete (void) {

  while (m_txn_pool.size() < m_active_txn_count){
    m_check_all = true; 
    check_complete();
    }
   m_check_all = false; 
} // end check_all_complete
