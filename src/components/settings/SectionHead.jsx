/**
 * Standard heading for each Settings section: icon chip + title + description.
 */
export default function SectionHead({ icon: Icon, title, desc, tone = 'blue' }) {
  return (
    <div className="settings-head">
      <div className={`stat-icon ${tone}`}>
        <Icon size={19} />
      </div>
      <div>
        <h2>{title}</h2>
        <p>{desc}</p>
      </div>
    </div>
  )
}