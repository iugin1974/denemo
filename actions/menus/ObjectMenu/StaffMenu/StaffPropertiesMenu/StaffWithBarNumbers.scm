;;StaffWithBarNumbers
(let ((tag "StaffWithBarNumbers"))
(d-DirectivePut-score-prefix tag
"\\layout {
		  \\context {
			\\Score
			\\remove \"Bar_number_engraver\"
			}
		}
")
(if (d-Directive-staff? tag)
	(begin
		(d-DirectiveDelete-staff tag)
		(d-WarningDialog (_ "Bar numbers removed from this staff. You may wish to put them on the top staff.")))
	(d-DirectivePut-staff-prefix tag "\\with { \\consists \"Bar_number_engraver\" }")))