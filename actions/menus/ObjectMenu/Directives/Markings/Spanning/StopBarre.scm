;StopBarre
(let ((tag "StopBarre"))
	(d-DirectivePut-standalone tag)
	(d-DirectivePut-standalone-display tag (_ "Stop Barre"))
	(d-DirectivePut-standalone-tx tag 20)
	(d-DirectivePut-standalone-minpixels tag 30)
       (d-DirectivePut-standalone-postfix tag "<>\\stopBarre ")
       (d-RefreshDisplay))
