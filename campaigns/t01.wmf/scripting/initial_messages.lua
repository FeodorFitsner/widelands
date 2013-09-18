-- ===============================
-- Send a bunch of story messages 
-- ===============================

function briefing_messages() 
-- Briefing message
title = _"The story begins"
msg = "<rt><p font-size=24 font-face=DejaVuSerif font-weight=bold font-color=2F9131>" ..
_"Thron sighs..." ..
"</p></rt><rt image=map:chieftain.png><p line-spacing=3 font-size=12>" ..
_[[ "It's been months and we are still hiding where the forests are old and dark.<br> My warriors hunt at day and lie awake at night - listening to the sounds of the cruel slaughter echoing from afar amongst the ancient trees." ]] .. 
"</p></rt>"
show_story_box(title, msg)

msg = 
"<rt image=map:chieftain.png><p line-spacing=3 font-size=12>" ..
_[[ "We can see the raging flames that swallow Al'thunran from here, miles away.<br> The red lights flash in the darkness and dance to the rhythm of the wardrums that haunt me even in my nightmares." ]] .. 
"</p></rt>"
show_story_box(title, msg, al_thunran)

msg = "<rt image=map:chieftain.png><p line-spacing=3 font-size=12>" ..
_[[ "My father's bones rest peacefully in the grounds on which he once ended the senseless bloodspilling that has risen among us. It pains me that his peace only endured for one generation.<br> Boldreth, my loyal companion and friend is a source of peace and comfort to me in these dark times. He keeps my spirits high and those of my warriors awake, not to let greed or despair destroy the bonds between us as well." ]] ..
"</p></rt>"
show_story_box(title, msg, grave)

msg = "<rt image=map:chieftain.png><p line-spacing=3 font-size=12>" ..
_[[ "As father told me, there are times to fight and times to lie and wait, trying not to fall asleep or die before the right time comes. And so I do... wait." ]] .. 
"</p></rt>"
show_story_box(title, msg)
sleep(2000)

use("map", "khantrukhs_talking")
end


run(briefing_messages)

