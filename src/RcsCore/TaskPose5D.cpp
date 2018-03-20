/*******************************************************************************

  Copyright (c) 2017, Honda Research Institute Europe GmbH.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement: This product includes
     software developed by the Honda Research Institute Europe GmbH.

  4. Neither the name of the copyright holder nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "TaskPose5D.h"
#include "TaskPosition3D.h"
#include "TaskPolar2D.h"
#include "TaskFactory.h"


static Rcs::TaskFactoryRegistrar<Rcs::TaskPose5D> registrar("XYZ-Polar");


/*******************************************************************************
 * Constructor based on xml parsing
 ******************************************************************************/
Rcs::TaskPose5D::TaskPose5D(const std::string& className_,
                            xmlNode* node,
                            RcsGraph* _graph):
  CompositeTask(className_, node, _graph)
{
  addTask(new TaskPosition3D("XYZ", node, _graph, 3));
  addTask(new TaskPolar2D("POLAR", node, _graph, 2));
}

/*******************************************************************************
 * See header.
 ******************************************************************************/
bool Rcs::TaskPose5D::isValid(xmlNode* node, const RcsGraph* graph)
{
  bool success = Rcs::Task::isValid(node, graph, "XYZ-Polar");

  return success;
}
