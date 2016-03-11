/*
 * xbar_sc_fifo_ifs.h
 *
 *  Created on: 7 Dec 2015
 *      Author: effiong
 */

#ifndef XBAR_SC_FIFO_IFS_H_
#define XBAR_SC_FIFO_IFS_H_

#include "systemc.h"
#include "sysc/communication/sc_interface.h"

namespace dreamcloud {
namespace platform_sclib {
// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_nonblocking_in_if<T>
//
//  The sc_fifo<T> input nonblocking interface class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_nonblocking_in_if: virtual public sc_interface {
public:

	// non-blocking read
	virtual bool nb_read(T&) = 0;
	virtual bool nb_read_(T&) = 0;

	// get the data written event
	virtual const sc_event& data_written_event() const = 0;
};


// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_nonblocking_in_if<T>
//
//  The sc_fifo<T> input nonblocking interface class.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  CLASS : sc_fifo_blocking_in_if<T>
//
//  The sc_fifo<T> input blocking interface class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_blocking_in_if: virtual public sc_interface {
public:

	// blocking read
	virtual void read(T&) = 0;
	virtual T read() = 0;
};

// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_in_if<T>
//
//  The sc_fifo<T> input interface class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_in_if: public xb_sc_fifo_nonblocking_in_if<T>,
		public xb_sc_fifo_blocking_in_if<T> {
public:

	// get the number of available samples
	virtual int num_available() const = 0;

protected:

	// constructor

	xb_sc_fifo_in_if() {
	}

private:

	// disabled
	xb_sc_fifo_in_if(const xb_sc_fifo_in_if<T>&);
	xb_sc_fifo_in_if<T>& operator =(const xb_sc_fifo_in_if<T>&);
};

// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_nonblocking_out_if<T>
//
//  The sc_fifo<T> nonblocking output interface class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_nonblocking_out_if: virtual public sc_interface {
public:

	// non-blocking write
	virtual bool nb_write(const T&) = 0;

	// get the data read event
	virtual const sc_event& data_read_event() const = 0;
};

// ----------------------------------------------------------------------------
//  CLASS : sc_fifo_blocking_out_if<T>
//
//  The sc_fifo<T> blocking output interface class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_blocking_out_if: virtual public sc_interface {
public:

	// blocking write
	virtual void write(const T&) = 0;

};

// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_out_if<T>
//
//  The sc_fifo<T> output interface class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_out_if: public xb_sc_fifo_nonblocking_out_if<T>,
		public xb_sc_fifo_blocking_out_if<T> {
public:

	// get the number of free spaces
	virtual int num_free() const = 0;

protected:

	// constructor

	xb_sc_fifo_out_if() {
	}

private:

	// disabled
	xb_sc_fifo_out_if(const xb_sc_fifo_out_if<T>&);
	xb_sc_fifo_out_if<T>& operator =(const xb_sc_fifo_out_if<T>&);
};
}
}

#endif /* XBAR_SC_FIFO_IFS_H_ */
