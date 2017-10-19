#  Copyright 2017 Patrick O. Perry.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.


with_rethrow <- function(expr)
{
    parentcall <- sys.call(-1)
    eval(envir = parent.frame(),
        withCallingHandlers(expr,
            error = function(e, call = parentcall) {
                e$call <- call
                stop(e)
            },
            warning = function(w, call = parentcall) {
                w$call <- call
                warning(w)
                invokeRestart("muffleWarning")
            },
            message = function(m, call = parentcall) {
                m$call <- call
            }
        )
    )
}
