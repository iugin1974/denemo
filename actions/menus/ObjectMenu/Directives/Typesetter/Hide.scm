(let ((lilytype (GetTypeAsLilypond))  (lilycontext  (GetContextAsLilypond)))
	(if (d-Directive-standalone?)
		(d-WarningDialog (_ "Use Conditional Directives menu to hide this directive"))
		(if lilytype
			(StandAloneDirectiveProto (cons "HideNext" (string-append  "\\once \\override " lilycontext "." lilytype ".stencil = ##f"  ))) #f)))
