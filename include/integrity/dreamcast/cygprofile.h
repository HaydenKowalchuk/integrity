/**
 * \file
 * \brief Interface to profile/function recorder.
 * \author Erich Styger
 *
 * With -finstrument-functions compiler option, each function entry and exit function
 * will call the hooks __cyg_profile_func_enter() and __cyg_profile_func_exit() which
 * can be used to trace function calls.
 * Functions which shall *not* be profiled/recorded need __attribute__((no_instrument_function)).
 */
#pragma once

#ifndef CYGPROFILE_H_
#define CYGPROFILE_H_

#include <kos.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef __PE_Error_H
#define __PE_Error_H

#define ERR_OK 0       /* OK */
#define ERR_SPEED 1    /* This device does not work in the active speed mode. */
#define ERR_RANGE 2    /* Parameter out of range. */
#define ERR_VALUE 3    /* Parameter of incorrect value. */
#define ERR_OVERFLOW 4 /* Timer overflow. */
#define ERR_MATH 5     /* Overflow during evaluation. */
#define ERR_ENABLED 6  /* Device is enabled. */
#define ERR_DISABLED 7 /* Device is disabled. */
#define ERR_BUSY 8     /* Device is busy. */
#define ERR_NOTAVAIL 9 /* Requested value or method not available. */
#define ERR_RXEMPTY 10 /* No data in receiver. */
#define ERR_TXFULL 11  /* Transmitter is full. */
#define ERR_BUSOFF 12  /* Bus not available. */
#define ERR_OVERRUN 13 /* Overrun error is detected. */
#define ERR_FRAMING 14 /* Framing error is detected. */
#define ERR_PARITY 15  /* Parity error is detected. */
#define ERR_NOISE 16   /* Noise error is detected. */
#define ERR_IDLE 17    /* Idle error is detectes. */
#define ERR_FAULT 18   /* Fault error is detected. */
#define ERR_BREAK 19   /* Break char is received during communication. */
#define ERR_CRC 20     /* CRC error is detected. */
#define ERR_ARBITR 21  /* A node losts arbitration. This error occurs if two nodes start transmission at the same time. */
#define ERR_PROTECT 22 /* Protection error is detected. */

#endif /* __PE_Error_H */

#define CYG_FUNC_TRACE_ENABLED (1)
/*!< 1: Trace enabled, 0: trace disabled */

/*!
 * \brief Print the call trace to the terminal.
 */
void CYG_PrintCallTrace(void);

/*!
 * \brief Driver Initialization.
 */
void CYG_Init(void);

/*!
 * \brief Driver De-Initialization.
 */
void CYG_Deinit(void);

#endif /* CYGPROFILE_H_ */
