This plugin will replace all shortened links with their full equivalents. It can also support expanding twitlonger shortened texts, as well as embed pictures from several popular media sharing sites.

Currently, the following sites are supported

tinyurl.com
bit.ly
t.co
is.gd
j.mp
goo.gl
ow.ly
tl.gd
twitlonger.com
twitpic.com
lockerz.com
yfrog.com
twitgoo.com (as of 0.4.0)
lightbox.com (as of 0.5.0)
picplz.com (as of 0.5.0)

The replacement is done in Pidgin, as a hook to the displaying-*-msg signal, so the log will have the original text. If used in conjunction with the prpltwtr plugin, the "Old Retweet" functionality will also retain the original.
