/*********************                                                        */
/*! \file proof_node_updater.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Morgan Deters, Andrew Reynolds, Tim King
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implementation of a utility for updating proof nodes
 **/

#include "expr/proof_node_updater.h"

#include "expr/lazy_proof.h"
#include "expr/proof_node_algorithm.h"

namespace CVC4 {

ProofNodeUpdaterCallback::ProofNodeUpdaterCallback() {}
ProofNodeUpdaterCallback::~ProofNodeUpdaterCallback() {}

bool ProofNodeUpdaterCallback::update(Node res,
                                      PfRule id,
                                      const std::vector<Node>& children,
                                      const std::vector<Node>& args,
                                      CDProof* cdp)
{
  return false;
}

ProofNodeUpdater::ProofNodeUpdater(ProofNodeManager* pnm,
                                   ProofNodeUpdaterCallback& cb)
    : d_pnm(pnm), d_cb(cb)
{
}

void ProofNodeUpdater::process(std::shared_ptr<ProofNode> pf)
{
  Trace("pf-process") << "ProofNodeUpdater::process" << std::endl;
  std::unordered_map<ProofNode*, bool> visited;
  std::unordered_map<ProofNode*, bool>::iterator it;
  std::vector<ProofNode*> visit;
  ProofNode* cur;
  visit.push_back(pf.get());
  std::map<Node, ProofNode*>::iterator itc;
  std::map<Node, ProofNode*> resCache;
  // NOTE: temporary, debugging
  unsigned counterReuse = 0;
  unsigned counterNew = 0;
  TNode res;
  do
  {
    cur = visit.back();
    visit.pop_back();
    it = visited.find(cur);
    res = cur->getResult();
    if (it == visited.end())
    {
      itc = resCache.find(res);
      if (itc != resCache.end())
      {
        counterReuse++;
        // already have a proof
        visited[cur] = true;
        d_pnm->updateNode(cur, itc->second);
      }
      else
      {
        counterNew++;
        visited[cur] = false;
        // run update first
        runUpdate(cur);
        visit.push_back(cur);
        const std::vector<std::shared_ptr<ProofNode>>& ccp = cur->getChildren();
        // now, process children
        for (const std::shared_ptr<ProofNode>& cp : ccp)
        {
          if (cp->getRule() == PfRule::SCOPE)
          {
            // process in new scope separately
            process(cp);
            continue;
          }
          visit.push_back(cp.get());
        }
      }
      Trace("pf-process-debug")
          << "Processing " << counterReuse << "/" << counterNew << std::endl;
    }
    else if (!it->second)
    {
      visited[cur] = true;
      // cache result
      resCache[res] = cur;
    }
  } while (!visit.empty());
  Trace("pf-process") << "ProofNodeUpdater::process: finished" << std::endl;
}

void ProofNodeUpdater::runUpdate(ProofNode* cur)
{
  // should it be updated?
  if (!d_cb.shouldUpdate(cur))
  {
    return;
  }
  PfRule id = cur->getRule();
  // use CDProof to open a scope for which the callback updates
  CDProof cpf(d_pnm);
  const std::vector<std::shared_ptr<ProofNode>>& cc = cur->getChildren();
  std::vector<Node> ccn;
  for (const std::shared_ptr<ProofNode>& cp : cc)
  {
    Node cpres = cp->getResult();
    ccn.push_back(cpres);
    // store in the proof
    cpf.addProof(cp);
  }
  Trace("pf-process-debug")
      << "Updating (" << cur->getRule() << ")..." << std::endl;
  Node res = cur->getResult();
  // only if the callback updated the node
  if (d_cb.update(res, id, ccn, cur->getArguments(), &cpf))
  {
    std::shared_ptr<ProofNode> npn = cpf.getProofFor(res);
    // this may not be a desired assertion in general, but typically
    if (cur->isClosed() && !npn->isClosed())
    {
      std::stringstream ss;
      ss << "Updated to non-closed proof: " << std::endl;
      cur->printDebug(ss);
      ss << std::endl << " to " << std::endl;
      npn->printDebug(ss);
      ss << std::endl;
      std::map<Node, std::vector<ProofNode*>> famap;
      expr::getFreeAssumptionsMap(npn.get(), famap);
      for (const std::pair<const Node, std::vector<ProofNode*>>& aprint : famap)
      {
        ss << "- new assumption: " << aprint.first << std::endl;
      }
      AlwaysAssert(false) << ss.str();
    }
    // then, update the original proof node based on this one
    Trace("pf-process-debug") << "Update node..." << std::endl;
    d_pnm->updateNode(cur, npn.get());
    Trace("pf-process-debug") << "...update node finished." << std::endl;
  }
  Trace("pf-process-debug") << "..finished" << std::endl;
}

}  // namespace CVC4
