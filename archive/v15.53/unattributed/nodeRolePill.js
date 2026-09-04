// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeRolePill(L){
  var badge=nodeRoleBadge(L); if(!badge) return '';
  // pull the label + color out of the badge span (cheap: re-derive from nodeRoleBadge's rules)
  // (v10.36) crown-only for the King (drop the ★ when 👑 present, for column alignment);
  // ★ still marks a strongest floor/ceiling that is NOT the King.
  var icon=(L.isKing?'\uD83D\uDC51':'')+(L.isGatekeeper?gateSvgSm(PAL.amber):'')+((L.isRugTarget||L.isRugCeil||L.isRugFloor)?'\uD83E\uDDF6':'');
  if(!icon) return badge; // Floor/Ceiling/Pika/Barn have no icon — the word badge stands alone
  // inject the icon right after the opening '>' of the badge span
  return badge.replace(/>([^<]*)<\/span>$/, function(_,txt){ return '>'+icon+' '+txt+'</span>'; });
}
