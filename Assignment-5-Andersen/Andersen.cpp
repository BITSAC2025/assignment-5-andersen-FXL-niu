/**
 * Andersen.cpp
 * @author kisslune
 */

 #include "A5Header.h"

 using namespace llvm;
 using namespace std;
 
 int main(int argc, char** argv)
 {
     auto moduleNameVec =
             OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                      "[options] <input-bitcode...>");
 
     SVF::LLVMModuleSet::buildSVFModule(moduleNameVec);
 
     SVF::SVFIRBuilder builder;
     auto pag = builder.build();
     auto consg = new SVF::ConstraintGraph(pag);
     consg->dump();
 
     Andersen andersen(consg);
 
     // TODO: complete the following method
     andersen.runPointerAnalysis();
 
     andersen.dumpResult();
     SVF::LLVMModuleSet::releaseLLVMModuleSet();
     return 0;
 }
 
 
 void Andersen::runPointerAnalysis()
 {
     // TODO: complete this method. Point-to set and worklist are defined in A5Header.h
     //  The implementation of constraint graph is provided in the SVF library
     WorkList<SVF::NodeID> worklist;
 
     for (auto const& nodePair : *consg) {
         SVF::ConstraintNode* node = nodePair.second;
         SVF::NodeID nodeId = node->getId();
         for (const auto& edge : node->getDirectOutEdges()) {
             if (edge->getEdgeKind() == SVF::ConstraintEdge::Addr) {
                 SVF::NodeID obj = edge->getSrcID();
                 SVF::NodeID ptr = edge->getDstID();      
                 if (pts[ptr].insert(obj).second) {
                     worklist.push(ptr);
                 }
             }
         }
     }
     while (!worklist.empty()) {
         SVF::NodeID nodeId = worklist.pop();
         SVF::ConstraintNode* node = consg->getConstraintNode(nodeId);
         
         std::set<unsigned>& nodePts = pts[nodeId]; 
         
         if (nodePts.empty()) continue;
         std::vector<std::pair<SVF::NodeID, SVF::NodeID>> newCopyEdges;
 
         for (const auto& edge : node->getDirectOutEdges()) {
             SVF::NodeID dstId = edge->getDstID();
             
             if (edge->getEdgeKind() == SVF::ConstraintEdge::Copy) {
                 bool changed = false;
                 for (auto obj : nodePts) {
                     if (pts[dstId].insert(obj).second) {
                         changed = true;
                     }
                 }
                 if (changed) worklist.push(dstId);
             }
             else if (edge->getEdgeKind() == SVF::ConstraintEdge::Load) {
                 for (auto obj : nodePts) {
                     newCopyEdges.push_back({obj, dstId});
                 }
             }
         }
         for (const auto& edge : node->getDirectInEdges()) {
             if (edge->getEdgeKind() == SVF::ConstraintEdge::Store) {
                 SVF::NodeID srcId = edge->getSrcID(); // q
                 for (auto obj : nodePts) { 
                     newCopyEdges.push_back({srcId, obj});
                 }
             }
         }
 
         for (const auto& pair : newCopyEdges) {
             SVF::NodeID src = pair.first;
             SVF::NodeID dst = pair.second;
             consg->addCopyCGEdge(src, dst);
             bool changed = false;
             for (auto obj : pts[src]) {
                 if (pts[dst].insert(obj).second) {
                     changed = true;
                 }
             }
             if (changed) worklist.push(dst);
         }
     }
 }