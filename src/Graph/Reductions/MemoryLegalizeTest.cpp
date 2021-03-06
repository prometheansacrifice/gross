#include "gross/Graph/Reductions/MemoryLegalize.h"
#include "gross/Graph/Reductions/ValuePromotion.h"
#include "gross/Graph/Reductions/Peephole.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace gross;

TEST(GRMemoryLegalizeUnitTest, DLXMemLegalizeTest) {
  {
    Graph G;
    // local alloca merging
    auto* Func = NodeBuilder<IrOpcode::VirtFuncPrototype>(&G)
                 .FuncName("func_dlx_mem_legal_alloca_merge")
                 .Build();
    auto* ArrayDecl = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                      .SetSymbolName("barArray")
                      .AddConstDim(9)
                      .AddConstDim(4)
                      .Build();
    auto* ArrayDecl2 = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                       .SetSymbolName("fooArray")
                       .AddConstDim(8)
                       .AddConstDim(7)
                       .Build();
    auto* Pred = NodeBuilder<IrOpcode::ConstantInt>(&G, 1).Build();
    auto* IfBranch = NodeBuilder<IrOpcode::If>(&G)
                     .Condition(Pred).Build();
    auto* IfTrue = NodeBuilder<IrOpcode::VirtIfBranches>(&G, true)
                   .IfStmt(IfBranch)
                   .Build();
    auto* IfFalse = NodeBuilder<IrOpcode::VirtIfBranches>(&G, false)
                    .IfStmt(IfBranch)
                    .Build();

    auto* Dim1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 4).Build();
    auto* Dim2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 5).Build();
    auto* Dim3 = NodeBuilder<IrOpcode::ConstantInt>(&G, 6).Build();
    auto* Dim4 = NodeBuilder<IrOpcode::ConstantInt>(&G, 7).Build();

    auto* ArrayAccess1 = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                        .Decl(ArrayDecl)
                        .AppendAccessDim(Dim1)
                        .AppendAccessDim(Dim2)
                        .Build();
    auto* RHSVal1 = NodeBuilder<IrOpcode::ConstantInt>(&G, 2).Build();
    auto* Assign1 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                   .Dest(ArrayAccess1).Src(RHSVal1)
                   .Build();

    auto* ArrayAccess2 = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                        .Decl(ArrayDecl2)
                        .AppendAccessDim(Dim3)
                        .AppendAccessDim(Dim4)
                        .Build();
    auto* RHSVal2 = NodeBuilder<IrOpcode::ConstantInt>(&G, 3).Build();
    auto* Assign2 = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                   .Dest(ArrayAccess2).Src(RHSVal2)
                   .Build();

    auto* Merge = NodeBuilder<IrOpcode::Merge>(&G)
                  .AddCtrlInput(IfTrue).AddCtrlInput(IfFalse)
                  .Build();
    auto* PHINode = NodeBuilder<IrOpcode::Phi>(&G)
                    .SetCtrlMerge(Merge)
                    .AddEffectInput(Assign1)
                    .AddEffectInput(Assign2)
                    .Build();

    auto* End = NodeBuilder<IrOpcode::End>(&G, Func)
                .AddTerminator(Merge)
                .AddEffectDep(PHINode)
                .Build();
    SubGraph FuncSG(End);
    G.AddSubRegion(FuncSG);
    GraphReducer::RunWithEditor<ValuePromotion>(G);
    {
      std::ofstream OF("TestDLXMemLegalize.mem2reg.dot");
      G.dumpGraphviz(OF);
    }

    DLXMemoryLegalize MemLegalize(G);
    MemLegalize.Run();
    {
      std::ofstream OF("TestDLXMemLegalize.mem2reg.after.dot");
      G.dumpGraphviz(OF);
    }

    GraphReducer::RunWithEditor<PeepholeReducer>(G);
    {
      std::ofstream OF("TestDLXMemLegalize.mem2reg.legal.ph.dot");
      G.dumpGraphviz(OF);
    }
  }
}
