/**
 * Form primitives: Field (label + error), Input, Select, Textarea.
 * Using these keeps every form in the app consistent.
 */

export function Field({ label, required, error, hint, children, htmlFor }) {
  return (
    <div className="form-group">
      {label && (
        <label className="label" htmlFor={htmlFor}>
          {label}
          {required && <span className="req">*</span>}
        </label>
      )}
      {children}
      {error && <div className="field-error">{error}</div>}
      {!error && hint && <div className="field-hint">{hint}</div>}
    </div>
  )
}

export function Input({ className = '', error, ...props }) {
  return <input className={`input ${error ? 'error' : ''} ${className}`.trim()} {...props} />
}

export function Select({ className = '', error, children, ...props }) {
  return (
    <select className={`select ${error ? 'error' : ''} ${className}`.trim()} {...props}>
      {children}
    </select>
  )
}

export function Textarea({ className = '', error, ...props }) {
  return <textarea className={`textarea ${error ? 'error' : ''} ${className}`.trim()} {...props} />
}
