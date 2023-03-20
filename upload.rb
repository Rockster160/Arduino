require "fileutils"
require "pry-rails"

D0 = :D0
D5 = :D5
D6 = :D6
D7 = :D7
D8 = :D8
TX = :TX
RX = :RX
D1 = :D1
D2 = :D2
D3 = :D3
D4 = :D4

class Btn
  def self.channel=(set_channel); @channel = set_channel; end
  def self.wifi=(set_wifi); @wifi = set_wifi; end
  def self.led=(pins); @ledpins = pins; end
  def self.btns=(pin_data); @btndata = pin_data; end

  def self.channel = @channel || :demo
  def self.wifi
    @wifi ||= :basement # :upstairs
    "#{@wifi.to_s.capitalize}Wifi"
  end
  def self.led = @ledpins || [D0, D7, D8]
  def self.btns = @btndata || { blu: D4, org: D6, yel: D3, red: D2, wht: D1, grn: D5 }
end

# ==================================================================================================

class Desk < Btn
  # channel = :ring
  # wifi = :basement
  led = [D0, D7, D8]
  btns = {
    busp: D4,
    water: D6,
    soda: D3,
    protein: D2,
  }
end

# class Grocery < Btn
# end

# ==================================================================================================

def fail!(msg)
  puts "\e[31m#{msg}\e[0m"
  exit
end

def replace(oldstr, newstr)
  found = false
  if oldstr.end_with?("{")
    idx = @raw.index(oldstr)
    unless idx.nil?
      found = true
      endidx = @raw[idx..].index("};")
      @raw[idx..idx+endidx] = newstr
    end
  else
    @raw = @raw.map { |line|
      if line == oldstr
        found = true
        newstr
      else
        line
      end
    }
  end

  fail!("Unable to replace string- not found: '#{oldstr}'") unless found
end

src = "/Users/rocco/Documents/Arduino/espBtn/espBtn.ino"
backup = "/Users/rocco/Documents/Arduino/espBtn/espBtn.backup"

begin
  klass = Object.const_get(ARGV[1].capitalize)
rescue NameError
  fail!("No type found for '#{ARGV[1]}'")
end

# Raise if "uploading" comment is found in src
@raw = File.read(src).split("\n")

# fail!("File is in incomplete state. May need to restore backup.") if @raw.first == "// uploading..."
#
# # Create backup of original file
# FileUtils.cp(src, backup)
# # Add `uploading` code so that we know the new file is modified
# @raw.prepend("// uploading...")
#
# # Go through and replace with the new data
# replace(
#   "const String channelId = \"demo\";",
#   "const String channelId = \"#{klass.channel}\";",
# )
# replace(
#   "const int wifi = BasementWifi; // BasementWifi UpstairsWifi",
#   "const int wifi = #{klass.wifi};"
# )
# replace(
#   "const int btnCount = 6;",
#   "const int btnCount = #{klass.btns.length};"
# )
# replace(
#   "const String btnIds[btnCount] = {",
#   "const String btnIds[btnCount] = { #{klass.btns.keys.map { |v| "\"#{v}\"" }.join(", ")} };"
# )
# replace(
#   "const int btns[btnCount] = {",
#   "const int btns[btnCount] = { #{klass.btns.values.join(", ")} };"
# )
# replace(
#   "const int colorPins[3] = { D0, D7, D8 };",
#   "const int colorPins[3] = { #{klass.led.join(", ")} };"
# )
# 
# # Save the modified file
# File.write(src, @raw.join("\n"))
# Upload to the ESP
`arduino-cli compile --fqbn esp8266:esp8266:d1_mini espBtn && arduino-cli upload -p /dev/cu.usbserial-110 --fqbn esp8266:esp8266:d1_mini espBtn`
# Restore the original file
File.write(src, File.read(backup))
