return function(M,U,kinds,g)
 if g.turn<1 or g.turn>2 or g.phase<0 or g.phase>3 or g.step<0 or g.step>5 or g.at<3 or g.at>59 or g.round<1 or g.extra<0 or g.extra>4 or g.ender<0 or g.ender>2 then return false end
 if not U.array(g.place,58,0,7) or not U.array(g.seq,58,0,255) or not U.array(g.drawn,2,0,58) or not U.array(g.score,2,0,255) then return false end
 local d=M.deck(g.seed);for i=1,58 do if (g.place[d[i]]==0)~=(i>=g.at) then return false end end
 if g.step==1 and g.phase<3 then if g.drawn[1]==g.drawn[2] or g.place[g.drawn[1]]~=7 or g.place[g.drawn[2]]~=7 then return false end end
 if g.step==2 and g.place[g.pending]~=7 then return false end
 return true
end
