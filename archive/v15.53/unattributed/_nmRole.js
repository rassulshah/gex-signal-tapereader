// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function _nmRole(L){ if(!L) return 'node'; if(L.isKing) return 'King'; if(L.isGatekeeper) return 'gate'; if(L.isFlr) return 'floor'; if(L.isCeil) return 'ceiling'; return 'node'; }
