;;;NonPrintingVoice
(let ((tag "NonPrintingVoice")(params NonPrintingVoice::params));(disp "params " params " so eq set? " (eq? params 'set) (eq? params 'unset) "\n")
     (if (eq? params 'unset)
     	(begin ;(disp "deleting for " params "\n") 
     		(d-DirectiveDelete-voice tag))
     	(begin
     		(if (eq? params 'set)
     			(d-DirectiveDelete-voice tag))
		(if (d-DirectiveGetForTag-voice tag)
			(begin
				(if (not params)
					(d-InfoDialog (_ "Voice will be printed")))
				(d-DirectiveDelete-voice tag))
			(begin 
				(d-DirectivePut-voice-prefix tag " {} \\void ")
				(d-DirectivePut-voice-override tag  (logior DENEMO_ALT_OVERRIDE DENEMO_OVERRIDE_GRAPHIC))
				(d-DirectivePut-voice-display tag "Hidden Staff")
				(if (not params)
					(SetDirectiveConditional "voice"  tag))))
				(d-SetSaved #f))))
