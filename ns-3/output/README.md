# ndn_output
result_20170607_v1.txt：Eason师兄代码，PIT设置了10秒钟的超时机制
result_20170607_v2.txt: Eason师兄代码，没有超时机制

output_20170625.txt 同output_20170626.txt
output_20170626.txt 发送兴趣包的条件为：m_nbChange_mode > 1 || lostForwardNeighbor
(当邻居数目增加或邻居发生变化或转发节点丢失，此时还未设置m_nbChange_mode == 4)
output_20170626_2.txt 发送兴趣包的条件为: m_nbChange_mode == 4 || lostForwardNeighbor
(还不存在转发节点或转发节点丢失)
output_20170626_2.txt 发送兴趣包的条件为: m_nbChange_mode > 1 || lostForwardNeighbor
(当邻居数目增加或邻居发生变化或转发节点丢失，此时已经设置m_nbChange_mode == 4)
