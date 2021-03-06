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

#include "TaskPolarSurfaceNormal.h"
#include "TaskFactory.h"
#include "Rcs_typedef.h"
#include "Rcs_macros.h"
#include "Rcs_parser.h"
#include "Rcs_math.h"
#include "Rcs_kinematics.h"
#include "Rcs_body.h"


// register task at the task factory
static Rcs::TaskFactoryRegistrar<Rcs::TaskPolarSurfaceNormal> registrar("POLAR_SURFACE");



/*******************************************************************************
 * Constructor based on xml parsing
 ******************************************************************************/
Rcs::TaskPolarSurfaceNormal::TaskPolarSurfaceNormal(const std::string& className,
                                                    xmlNode* node,
                                                    RcsGraph* _graph,
                                                    int dim) :
  Task(className, node, _graph, dim), direction(2), gainDX(1.0)
{
  if (getClassName()=="POLAR_SURFACE")
  {
    resetParameter(Parameters(-M_PI, M_PI, (180.0/M_PI), "Phi [deg]"));
    addParameter(Parameters(-M_PI, M_PI, (180.0/M_PI), "Theta [deg]"));
  }

  // Parse axis direction (should be X, Y or Z)
  char text[256] = "Z";
  getXMLNodePropertyStringN(node, "axisDirection", text, 256);

  if (STRCASEEQ(text, "X"))
  {
    this->direction = 0;
  }
  else if (STRCASEEQ(text, "Y"))
  {
    this->direction = 1;
  }
  else if (STRCASEEQ(text, "Z"))
  {
    this->direction = 2;
  }

  // The gainDX scales the position error coming out of computeDX. It is
  // sometimes needed to scale it down to avoid jitter due to the contact
  // normal updates.
  getXMLNodePropertyDouble(node, "gainDX", &this->gainDX);
}

/*******************************************************************************
 *
 ******************************************************************************/
Rcs::TaskPolarSurfaceNormal::TaskPolarSurfaceNormal(const TaskPolarSurfaceNormal& copyFromMe,
                                                    RcsGraph* newGraph):
  Task(copyFromMe, newGraph), direction(copyFromMe.direction),
  gainDX(copyFromMe.gainDX)
{
}

/*******************************************************************************
 * Virtual destructor required for Polymorphism
 ******************************************************************************/
Rcs::TaskPolarSurfaceNormal::~TaskPolarSurfaceNormal()
{
}

/*******************************************************************************
 * Clone function required for Polymorphism
 ******************************************************************************/
Rcs::TaskPolarSurfaceNormal* Rcs::TaskPolarSurfaceNormal::clone(RcsGraph* newGraph) const
{
  return new Rcs::TaskPolarSurfaceNormal(*this, newGraph);
}

/*******************************************************************************
 * First element: angle between current and desired polar axis
 * Second element: 0
 ******************************************************************************/
void Rcs::TaskPolarSurfaceNormal::computeX(double* x_curr) const
{
  // Compute the normal pointing from the refBdy to the effector
  double a_des[3];
  RcsBody_distance(getRefBody(), getEffector(), NULL, NULL, a_des);

  // Get the current polar axis
  const double* a_curr = getEffector()->A_BI->rot[direction];

  x_curr[0] = Vec3d_diffAngle(a_curr, a_des);
  x_curr[1] = 0.0;
}

/*******************************************************************************
 *
 ******************************************************************************/
void Rcs::TaskPolarSurfaceNormal::computeDX(double* dx_ik,
                                            const double* x_des,
                                            const double* x_curr) const
{
  double phi_des = Math_clip(x_des[0], 0.0, M_PI);
  dx_ik[0] = this->gainDX*(x_curr[0] - phi_des);
  dx_ik[1] = 0.0;
}

/*******************************************************************************
 *
 ******************************************************************************/
void Rcs::TaskPolarSurfaceNormal::computeJ(MatNd* jacobian) const
{
  // Reshape to correct dimensions
  MatNd_reshape(jacobian, 2, this->graph->nJ);

  // Get the current Polar axis in world coordinates
  double* a_curr = ef->A_BI->rot[this->direction];

  // Compute the normal pointing from the refBdy to the effector. This is the
  // desired Polar axis in world coordinates.
  double a_des[3];
  RcsBody_distance(getRefBody(), getEffector(), NULL, NULL, a_des);

  double rotAxis[3];
  Vec3d_crossProduct(rotAxis, a_curr, a_des);
  double lengthRot = Vec3d_getLength(rotAxis);

  if (lengthRot > 0.0)
  {
    // Normalize rotation axis if not ill-defined
    Vec3d_constMulSelf(rotAxis, 1.0 / lengthRot);
  }
  else
  {
    Vec3d_orthonormalVec(rotAxis, a_curr);
  }

  double rhAxis[3];
  Vec3d_crossProduct(rhAxis, a_curr, rotAxis);

  MatNd* proj = NULL;
  MatNd_create2(proj, 2, 3);
  Vec3d_copy(&proj->ele[0], rotAxis);
  Vec3d_copy(&proj->ele[3], rhAxis);

  // Compute the angular velocity Jacobian
  MatNd* JR2 = NULL;
  MatNd_create2(JR2, 3, this->graph->nJ);
  RcsGraph_rotationJacobian(this->graph, this->ef, NULL, JR2);
  MatNd_mul(jacobian, proj, JR2);

  MatNd_destroy(proj);
  MatNd_destroy(JR2);
}

/*******************************************************************************
 *
 *******************************************************************************/
void Rcs::TaskPolarSurfaceNormal::computePolarNormal(double polarAngs[2]) const
{
  double n12[3];
  const RcsBody* b1 = getRefBody();
  const RcsBody* b2 = getEffector();
  RcsBody_distance(b1, b2, NULL, NULL, n12);
  Vec3d_getPolarAngles(polarAngs, n12);
}

/*******************************************************************************
 *
 ******************************************************************************/
bool Rcs::TaskPolarSurfaceNormal::isValid(xmlNode* node, const RcsGraph* graph)
{
  bool success = Rcs::Task::isValid(node, graph, "POLAR_SURFACE");

  char taskName[256] = "Unnamed task";
  getXMLNodePropertyStringN(node, "name", taskName, 256);

  // Check if axis direction is X, Y or Z
  char text[256] = "Z";
  getXMLNodePropertyStringN(node, "axisDirection", text, 256);

  if ((!STRCASEEQ(text, "X")) &&
      (!STRCASEEQ(text, "Y")) &&
      (!STRCASEEQ(text, "Z")))
  {
    RLOG(3, "Task \"%s\": Axis direction not [0...2]: %s",
         taskName, text);
    success = false;
  }

  // Check if there is a distance function called between effector and refBdy
  char name1[265] = "", name2[265] = "";
  getXMLNodePropertyStringN(node, "effector", name1, 256);
  getXMLNodePropertyStringN(node, "refBdy", name2, 256);
  getXMLNodePropertyStringN(node, "refBody", name2, 256);
  const RcsBody* b1 = RcsGraph_getBodyByName(graph, name1);
  const RcsBody* b2 = RcsGraph_getBodyByName(graph, name2);

  if (b1 == NULL)
  {
    RLOG(1, "Effector \"%s\" of task \"%s\" doesn't exist!", name1, taskName);
    success = false;
  }
  else if (b2 == NULL)
  {
    RLOG(1, "Ref-body \"%s\" of task \"%s\" doesn't exist!", name2, taskName);
    success = false;
  }
  else if (RcsBody_getNumDistanceQueries(b1, b2) == 0)
  {
    RLOG(1, "Task \"%s\": No distance function between \"%s\" and \"%s\"!",
         taskName, name1, name2);
    success = false;
  }

  return success;
}
