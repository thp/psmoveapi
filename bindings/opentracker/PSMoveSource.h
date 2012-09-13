/* ========================================================================
 *
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Released under the same terms as the OpenTracker library itself.
 *
 * Based on example code from:
 * Copyright (c) 2006,
 * Institute for Computer Graphics and Vision
 * Graz University of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the Graz University of Technology nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================
 * PROJECT: OpenTracker
 * ======================================================================== */

/**
 * @page Nodes Node Reference
 * @section psmovesource PSMoveSource
 *
 * The PSMoveSource node is an EventGenerator that outputs the
 * current position of the PS Move and button / trigger information.
 *
 * It is driven by the @ref psmovemodule.
 *
 * It has the following attributes:
 *
 * XXX ???
 **/

#ifndef _PSMOVESOURCE_H
#define _PSMOVESOURCE_H

#include "../OpenTracker.h"

#include "psmove.h"
#include "psmove_tracker.h"

#ifdef USE_PSMOVE

namespace ot {
    /**
     * This class implements a simple source that sets its valid flag in
     * regular intervals and updates any EventObservers.
     *
     * @author Thomas Perl
     *
     * Based on HydraSource by: Hannes Kaufmann, Istvan Barakonyi, Mathis Csisinko
     *
     * @ingroup input
     */
    class OPENTRACKER_API PSMoveSource : public Node
    {
        friend class PSMoveModule;

        private:
            Event event;
            PSMove *move;
            PSMoveTracker *tracker;

        protected:
            PSMoveSource(PSMoveTracker *tracker, int controller);
            ~PSMoveSource();

            /**
             * Tests for EventGenerator interface being present.
             *
             * @return always 1
             **/
            int
            isEventGenerator() { return 1; }

            Event &
            getEvent();
    };

} /* namespace ot */

#endif /* USE_PSMOVE */

#endif /* _PSMOVESOURCE_H */
