import stanza
import os
from typing import Dict, List, Any, Union, Optional, Tuple
from pathlib import Path
import logging
import torch
import re
from tqdm import tqdm
import time
import numpy as np
import nltk
from sklearn.metrics.pairwise import cosine_similarity
from sklearn.feature_extraction.text import TfidfVectorizer

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class NLPProcessor:
    """使用Stanza进行自然语言处理的通用处理器"""
    
    def __init__(self, language: str = 'zh', use_gpu: bool = True, download_if_missing: bool = True, batch_size: int = 32):
        """
        初始化NLP处理器
        
        Args:
            language: 处理语言，'zh'表示中文
            use_gpu: 是否使用GPU加速
            download_if_missing: 如果模型不存在是否自动下载
            batch_size: 批处理大小
        """
        self.language = language
        
        # 检查Stanza资源目录是否存在
        stanza_dir = Path.home() / 'stanza_resources'
        if not stanza_dir.exists() and download_if_missing:
            logger.info(f"初次使用Stanza，将下载{language}语言模型...")
        
        # 设置处理器配置
        processors = "tokenize,pos,lemma,ner"
        
        # 设置GPU使用
        gpu_available = torch.cuda.is_available()
        self.device = "cuda" if use_gpu and gpu_available else "cpu"
        logger.info(f"使用{self.device}处理{language}语言文本")
        
        if self.device == "cuda":
            self.batch_size = batch_size
        else:
            self.batch_size = batch_size
            
        try:
            # 初始化Stanza管道
            self.nlp = stanza.Pipeline(
                lang=language,
                processors=processors,
                use_gpu=(self.device == "cuda"),
                download_if_missing=download_if_missing,
                ner_batch_size=self.batch_size,
                logging_level='INFO'
            )
            logger.info(f"{language}语言处理器初始化完成")
        except Exception as e:
            logger.error(f"初始化Stanza处理器时出错: {e}")
            raise
    
    def process(self, text: str) -> Dict[str, Any]:
        """
        处理单个文本并返回分析结果
        
        Args:
            text: 要处理的文本
            
        Returns:
            包含分句、分词和命名实体的字典
        """
        if not text or not text.strip():
            logger.warning("处理空文本")
            return {"sentences": [], "tokens": [], "entities": []}
        
        try:
            # 使用Stanza处理文本
            start_time = time.time()
            doc = self.nlp(text)
            processing_time = time.time() - start_time
            
            # 提取句子
            sentences = [sentence.text for sentence in doc.sentences]
            
            # 提取分词结果
            tokens = [[token.text for token in sentence.tokens] for sentence in doc.sentences]
            
            # 提取词性标注
            pos_tags = [[word.pos for word in sentence.words] for sentence in doc.sentences]
            
            # 提取命名实体
            entities = []
            for ent in doc.entities:
                entities.append({
                    "text": ent.text,
                    "type": ent.type,
                    "start_char": ent.start_char,
                    "end_char": ent.end_char
                })
            
            # 返回处理结果
            return {
                "sentences": sentences,
                "tokens": tokens,
                "pos_tags": pos_tags,
                "entities": entities,
                "raw_doc": doc,
                "processing_time": processing_time
            }
            
        except Exception as e:
            logger.error(f"处理文本时出错: {e}")
            return {"error": str(e)}
    
    def extract_specific_entities(self, doc: Dict[str, Any], entity_types: Optional[List[str]] = None) -> Dict[str, List[Dict[str, Any]]]:
        """
        从处理结果中提取特定类型的实体
        
        Args:
            doc: process方法返回的处理结果
            entity_types: 要提取的实体类型列表，如['PERSON', 'ORG', 'LOC']
                          如果为None，则提取所有类型
        
        Returns:
            按类型分组的实体字典
        """
        if "error" in doc:
            return {"error": doc["error"]}
        
        result = {}
        
        for entity in doc.get("entities", []):
            entity_type = entity["type"]
            
            # 如果指定了类型且当前实体类型不在列表中，则跳过
            if entity_types and entity_type not in entity_types:
                continue
            
            if entity_type not in result:
                result[entity_type] = []
            
            result[entity_type].append(entity)
        
        return result
    


class ChineseProcessor(NLPProcessor):
    """专门处理中文文本的处理器"""
    
    def __init__(self, use_gpu: bool = True, download_if_missing: bool = True, batch_size: int = 32):
        """初始化中文处理器"""
        super().__init__(language='zh', use_gpu=use_gpu, download_if_missing=download_if_missing, batch_size=batch_size)
    
    def process(self, text: str) -> Dict[str, Any]:
        """处理中文文本"""
        result = super().process(text)
        
        # 针对中文的特殊处理
        if "error" not in result:
            # 提取中文特有的实体类型
            chinese_specific_entities = self.extract_specific_entities(
                result, 
                ['PERSON', 'LOC', 'ORG', 'GPE', 'TIME', 'DATE']
            )
            result["chinese_entities"] = chinese_specific_entities
        
        return result


class LegalDocumentProcessor(ChineseProcessor):
    """专门处理法律文档的处理器，抽取关键法律信息点"""
    
    def __init__(self, use_gpu: bool = True, download_if_missing: bool = True, batch_size: int = 32):
        super().__init__(use_gpu=use_gpu, download_if_missing=download_if_missing, batch_size=batch_size)
        
        # 法律角色关键词及其相似词
        self.legal_roles = {
            "原告": ["原告", "起诉方", "申请人", "申请执行人", "权利人", "权利方"],
            "被告": ["被告", "被诉方", "被执行人", "被申请人", "义务方", "当事人"],
            "代理人": ["委托代理人", "代理人", "律师", "辩护人", "诉讼代理人"],
            "法官": ["审判长", "审判员", "法官", "主审法官", "合议庭成员"],
            "书记员": ["书记员", "记录员", "法庭记录员"],
            "执行长": ["执行长"],
            "执行员": ["执行员", "执行法官", "执行人员"]
        }
        
        # 案件类型关键词
        self.case_types = {
            "民事": ["民事", "民初", "民终", "民执"],
            "刑事": ["刑事", "刑初", "刑终"],
            "行政": ["行政", "行初", "行终"],
            "执行": ["执行", "执", "执恢"]
        }
        
        # 编译常用正则表达式
        self.case_number_pattern = re.compile(r'[（\(](\d{4})[）\)][^\)]+字第\d+号')
        self.date_pattern = re.compile(r'(\d{4}年\d{1,2}月\d{1,2}日|\d{1,2}月\d{1,2}日)')
        self.money_pattern = re.compile(r'[壹贰叁肆伍陆柒捌玖拾佰仟万亿零元整\d]+元')
        self.law_article_pattern = re.compile(r'《([^》]+)》[^第]*第([零一二三四五六七八九十百千]+)条')
        
        # 特定角色模式
        self.plaintiff_pattern = re.compile(r'申请执行人[：:]\s*([\u4e00-\u9fa5]{2,3})[\s，·]')
        self.defendant_pattern = re.compile(r'被执行人[：:]\s*([\u4e00-\u9fa5]{2,3})[\s，·]')
        self.rights_holder_pattern = re.compile(r'权利人\s*([\u4e00-\u9fa5]{2,3})')

        # 法律人员模式
        self.execution_personnel_pattern = re.compile(
            r'执行长\s*([\u4e00-\u9fa5]{2,3})\s*执行员\s*([\u4e00-\u9fa5]{2,3})\s*执行员\s*([\u4e00-\u9fa5]{2,3})'
        )
        self.clerk_pattern = re.compile(r'书记员\s*([\u4e00-\u9fa5]{2,3})')
    
    def extract_legal_info(self, text: str) -> Dict[str, Any]:
        """
        从法律文档中抽取关键信息点，形成因果关系链
        
        抽取的5个关键信息点及其因果关系：
        1. 案件基本信息 (案号、法院、案件类型) -> 案件标识
        2. 当事人信息 (原告、被告、代理人) -> 案件主体
        3. 法律依据 (引用的法律条文) -> 法律基础
        4. 执行情况 (执行结果、执行措施) -> 案件结果
        5. 时间节点 (申请时间、裁定时间) -> 时间线
        
        这些信息点共同构成完整的事件链条
        """
        # 先进行基础NLP处理
        nlp_result = self.process(text)
        if "error" in nlp_result:
            return {"error": nlp_result["error"]}
        
        # 提取关键信息点
        legal_info = {
            "case_info": self._extract_case_info(text),
            "parties": self._extract_parties_info(text, nlp_result),
            "legal_basis": self._extract_legal_basis(text),
            "execution": self._extract_execution_info(text),
            "timeline": self._extract_timeline(text),
            "court": self._extract_court_name(text),
            "legal_personnel": self._extract_legal_personnel(text, nlp_result)
        }
        
        # 构建因果关系描述
        legal_info["causal_chain"] = self._build_causal_chain(legal_info)
        
        return legal_info
    
    def _extract_case_info(self, text: str) -> Dict[str, str]:
        """提取案件基本信息：案号、案件类型"""
        # 提取案号
        case_numbers = self.case_number_pattern.findall(text)
        main_case_number = case_numbers[0] if case_numbers else ""
        
        # 提取案件类型
        case_type = ""
        for ctype, keywords in self.case_types.items():
            for keyword in keywords:
                if keyword in text:
                    case_type = ctype
                    break
            if case_type:
                break
                
        # 寻找纠纷类型
        dispute_match = re.search(r'([\u4e00-\u9fa5]+纠纷)一案', text)
        dispute_type = dispute_match.group(1) if dispute_match else ""
        
        return {
            "case_number": main_case_number,
            "case_type": case_type,
            "dispute_type": dispute_type
        }
    
    def _extract_parties_info(self, text: str, nlp_result: Dict[str, Any]) -> List[Dict]:
        """提取当事人信息：原告、被告、代理人等"""
        parties = []
        identified_names = set()
        
        # 定义机构关键词，用于识别可能的长名称
        org_keywords = ["公司", "银行", "集团", "厂", "企业", "部门", "机构", "医院", "学校", "单位", "有限", "责任"]
        
        # 1. 匹配申请执行人
        plaintiff_pos = text.find("申请执行人")
        if plaintiff_pos != -1:
            # 先用宽松正则获取位置
            rough_match = re.search(r'申请执行人[：:]\s*([\u4e00-\u9fa5]+)(?:[，,。\.：:\s]|$)', text[plaintiff_pos:plaintiff_pos+200])
            if rough_match:
                # 初步匹配的文本
                potential_name = rough_match.group(1).strip()
                
                # 检查是否可能是机构名称
                is_org = any(keyword in potential_name for keyword in org_keywords)
                
                # 根据是否为机构名采取不同的处理策略
                if is_org:
                    # 尝试找到机构名的边界（如逗号、句号等）
                    end_markers = ["，", "。", "：", ":", " ", "、", "）", ")", "\n"]
                    end_pos = len(potential_name)
                    for marker in end_markers:
                        marker_pos = potential_name.find(marker)
                        if marker_pos != -1 and marker_pos < end_pos:
                            end_pos = marker_pos
                    
                    plaintiff_name = potential_name[:end_pos].strip()
                    
                    # 机构的情况下，名称可能更长
                    if len(plaintiff_name) > 3 and len(plaintiff_name) <= 30:
                        parties.append({
                            "name": plaintiff_name,
                            "role": "原告",
                            "type": "organization",
                            "details": {}
                        })
                        identified_names.add(plaintiff_name)
                else:
                    # 对于个人名字，采用严格的2-4字符限制
                    person_match = re.search(r'申请执行人[：:]\s*([\u4e00-\u9fa5]{2,4})(?:[，,。\.：:\s]|$)', text[plaintiff_pos:plaintiff_pos+50])
                    if person_match:
                        plaintiff_name = person_match.group(1).strip()
                        details = self._extract_person_details(plaintiff_name, text)
                        parties.append({
                            "name": plaintiff_name,
                            "role": "原告",
                            "type": "person",
                            "details": details
                        })
                        identified_names.add(plaintiff_name)
        
        # 2. 类似地处理被执行人
        defendant_pos = text.find("被执行人")
        if defendant_pos != -1:
            rough_match = re.search(r'被执行人[：:]\s*([\u4e00-\u9fa5]+)(?:[，,。\.：:\s]|$)', text[defendant_pos:defendant_pos+200])
            if rough_match:
                potential_name = rough_match.group(1).strip()
                is_org = any(keyword in potential_name for keyword in org_keywords)
                
                if is_org:
                    end_markers = ["，", "。", "：", ":", " ", "、", "）", ")", "\n"]
                    end_pos = len(potential_name)
                    for marker in end_markers:
                        marker_pos = potential_name.find(marker)
                        if marker_pos != -1 and marker_pos < end_pos:
                            end_pos = marker_pos
                    
                    defendant_name = potential_name[:end_pos].strip()
                    
                    if len(defendant_name) > 3 and len(defendant_name) <= 30:
                        parties.append({
                            "name": defendant_name,
                            "role": "被告",
                            "type": "organization",
                            "details": {}
                        })
                        identified_names.add(defendant_name)
                else:
                    person_match = re.search(r'被执行人[：:]\s*([\u4e00-\u9fa5]{2,4})(?:[，,。\.：:\s]|$)', text[defendant_pos:defendant_pos+50])
                    if person_match:
                        defendant_name = person_match.group(1).strip()
                        details = self._extract_person_details(defendant_name, text)
                        parties.append({
                            "name": defendant_name,
                            "role": "被告",
                            "type": "person",
                            "details": details
                        })
                        identified_names.add(defendant_name)
        
        # 3. 如果上述方法未找到当事人，结合NER识别
        if not parties and "entities" in nlp_result:
            # 获取所有组织和人名实体
            all_entities = [e for e in nlp_result.get("entities", []) 
                        if e["type"] in ["PERSON", "ORG", "GPE"]]
            
            # 查找每个实体的上下文，确定角色
            for entity in all_entities:
                entity_text = entity["text"].strip()
                if len(entity_text) < 2 or entity_text in identified_names:
                    continue
                    
                # 获取实体上下文
                context_start = max(0, entity["start_char"] - 30)
                context_end = min(len(text), entity["end_char"] + 30)
                context = text[context_start:context_end]
                
                # 判断角色
                role = None
                if any(keyword in context for keyword in self.legal_roles["原告"]):
                    role = "原告"
                elif any(keyword in context for keyword in self.legal_roles["被告"]):
                    role = "被告"
                    
                if role:
                    entity_type = "organization" if entity["type"] == "ORG" or any(keyword in entity_text for keyword in org_keywords) else "person"
                    details = {} if entity_type == "organization" else self._extract_person_details(entity_text, text)
                    
                    parties.append({
                        "name": entity_text,
                        "role": role,
                        "type": entity_type,
                        "details": details
                    })
                    identified_names.add(entity_text)
        
        # 4. 尝试匹配"XXX与YYY纠纷一案"格式
        if len(parties) < 2:
            dispute_parties_match = re.search(r'([\u4e00-\u9fa5]+)与([\u4e00-\u9fa5]+)([\u4e00-\u9fa5]+纠纷)一案', text)
            if dispute_parties_match:
                plaintiff = dispute_parties_match.group(1).strip()
                defendant = dispute_parties_match.group(2).strip()
                
                if plaintiff not in identified_names:
                    is_org = any(keyword in plaintiff for keyword in org_keywords)
                    entity_type = "organization" if is_org else "person"
                    details = {} if entity_type == "organization" else self._extract_person_details(plaintiff, text)
                    
                    parties.append({
                        "name": plaintiff,
                        "role": "原告",
                        "type": entity_type,
                        "details": details
                    })
                    identified_names.add(plaintiff)
                
                if defendant not in identified_names:
                    is_org = any(keyword in defendant for keyword in org_keywords)
                    entity_type = "organization" if is_org else "person"
                    details = {} if entity_type == "organization" else self._extract_person_details(defendant, text)
                    
                    parties.append({
                        "name": defendant,
                        "role": "被告",
                        "type": entity_type,
                        "details": details
                    })
                    identified_names.add(defendant)
        
        return parties
    
    def _identify_role_type(self, text: str) -> str:
        """使用关键词匹配识别角色类型"""
        for role_type, keywords in self.legal_roles.items():
            for keyword in keywords:
                if keyword in text:
                    return role_type
        return ""
    
    def _extract_person_details(self, name: str, text: str) -> Dict[str, str]:
        """提取当事人详细信息"""
        # 查找当事人出现的上下文
        name_pos = text.find(name)
        if name_pos == -1:
            return {}
        
        # 提取上下文片段
        start = max(0, name_pos - 50)
        end = min(len(text), name_pos + len(name) + 150)
        context = text[start:end]
        
        # 提取性别
        gender = ""
        if "男" in context and "女" not in context:
            gender = "男"
        elif "女" in context and "男" not in context:
            gender = "女"
        
        # 提取出生日期
        birth_date = ""
        birth_match = re.search(r'(\d{4}年\d{1,2}月\d{1,2}日)出生', context)
        if birth_match:
            birth_date = birth_match.group(1)
        
        # 提取民族
        ethnicity = ""
        for ethnic in ["汉族", "回族", "满族", "藏族", "维吾尔族", "壮族", "蒙古族"]:
            if ethnic in context:
                ethnicity = ethnic
                break
        
        # 提取职业
        occupation = ""
        for job in ["无业", "职员", "工人", "农民", "教师", "律师", "干部", "职工"]:
            if job in context:
                occupation = job
                break
        
        return {
            "gender": gender,
            "birth_date": birth_date,
            "ethnicity": ethnicity,
            "occupation": occupation
        }
    
    def _extract_legal_personnel(self, text: str, nlp_result: Dict[str, Any]) -> List[Dict]:
        """提取法律工作人员信息：法官、书记员、执行员等"""
        personnel = []
        
        # 特殊处理：检测"执行长XXX执行员YYY执行员ZZZ"这种格式
        execution_match = self.execution_personnel_pattern.search(text)
        if execution_match:
            execution_chief = execution_match.group(1).strip()
            execution_officer1 = execution_match.group(2).strip()
            execution_officer2 = execution_match.group(3).strip()
            
            personnel.append({"name": execution_chief, "role": "执行长"})
            personnel.append({"name": execution_officer1, "role": "执行员"})
            personnel.append({"name": execution_officer2, "role": "执行员"})
            
            # 在同一段落中寻找书记员
            clerk_match = self.clerk_pattern.search(text[execution_match.end():execution_match.end()+50])
            if clerk_match:
                clerk_name = clerk_match.group(1).strip()
                personnel.append({"name": clerk_name, "role": "书记员"})
            
            return personnel
        
        # 如果没有找到特定格式，尝试分别匹配
        # 1. 匹配执行长
        execution_chief_matches = re.finditer(r'执行长\s*([\u4e00-\u9fa5]{2,3})', text)
        for match in execution_chief_matches:
            chief_name = match.group(1).strip()
            if chief_name and len(chief_name) <= 4:
                personnel.append({"name": chief_name, "role": "执行长"})
        
        # 2. 匹配执行员
        execution_officer_matches = re.finditer(r'执行员\s*([\u4e00-\u9fa5]{2,3})', text)
        for match in execution_officer_matches:
            officer_name = match.group(1).strip()
            if officer_name and len(officer_name) <= 4:
                personnel.append({"name": officer_name, "role": "执行员"})
        
        # 3. 匹配书记员
        clerk_matches = re.finditer(r'书记员\s*([\u4e00-\u9fa5]{2,3})', text)
        for match in clerk_matches:
            clerk_name = match.group(1).strip()
            if clerk_name and len(clerk_name) <= 4:
                personnel.append({"name": clerk_name, "role": "书记员"})
        
        # 如果上面的方法都没有找到，尝试使用NER结果
        if not personnel:
            # 找到文档末尾
            last_part = text[-200:] if len(text) > 200 else text
            
            # 使用NER识别的人名
            persons = [e for e in nlp_result.get("entities", []) if e["type"] == "PERSON"]
            
            # 找出出现在文档末尾的人名，这些可能是法律人员
            end_persons = [p for p in persons if p["start_char"] > len(text) - 200]
            
            for person in end_persons:
                person_name = person["text"]
                # 根据上下文推断角色
                context_start = max(0, person["start_char"] - 10)
                context_end = min(len(text), person["end_char"] + 10)
                context = text[context_start:context_end]
                
                if "执行长" in context:
                    personnel.append({"name": person_name, "role": "执行长"})
                elif "执行员" in context:
                    personnel.append({"name": person_name, "role": "执行员"})
                elif "书记员" in context:
                    personnel.append({"name": person_name, "role": "书记员"})
                elif "审判长" in context or "审判员" in context:
                    personnel.append({"name": person_name, "role": "法官"})
        
        return personnel
    
    def _extract_legal_basis(self, text: str) -> List[Dict]:
        """提取法律依据"""
        laws = []
        matches = self.law_article_pattern.findall(text)
        for match in matches:
            law_name, article = match
            laws.append({
                "law_name": law_name,
                "article": article
            })
        return laws
    
    def _extract_execution_info(self, text: str) -> Dict[str, Any]:
        """提取执行情况"""
        # 提取执行结果
        result = ""
        if "终结" in text and "执行" in text:
            result = "终结执行"
        elif "中止" in text and "执行" in text:
            result = "中止执行"
        elif "恢复" in text and "执行" in text:
            result = "恢复执行"
        elif "划拨" in text:
            result = "划拨财产"
        elif "驳回" in text and "申请" in text:
            result = "驳回申请"
        
        # 提取执行措施
        measures = []
        if "划拨" in text:
            # 提取划拨金额
            money_match = self.money_pattern.search(text)
            money = money_match.group(0) if money_match else ""
            measures.append(f"划拨存款: {money}")
        elif "查封" in text:
            measures.append("查封财产")
        elif "冻结" in text:
            measures.append("冻结账户")
        
        # 提取执行理由
        reason = ""
        if "无财产可供执行" in text:
            reason = "被执行人无财产可供执行"
        elif "暂无执行能力" in text:
            reason = "被执行人暂无执行能力"
        elif "有存款" in text or "有财产" in text:
            reason = "被执行人有可供执行财产"
        elif "申请人同意" in text:
            reason = "申请人同意延期执行"
        elif "未履行义务" in text:
            reason = "被执行人未履行义务"
        
        return {
            "result": result,
            "measures": measures,
            "reason": reason
        }
    
    def _extract_timeline(self, text: str) -> Dict[str, str]:
        """提取时间节点"""
        dates = self.date_pattern.findall(text)
        
        # 查找申请执行的日期
        application_date = ""
        app_match = re.search(r'于(\d{4}年\d{1,2}月\d{1,2}日).*?申请执行', text)
        if app_match:
            application_date = app_match.group(1)
        
        # 查找裁定日期
        ruling_date = ""
        # 首先尝试查找"二零零X年X月X日"格式
        cn_date_match = re.search(r'二[零〇][\u4e00-\u9fa5]{1,2}年[\u4e00-\u9fa5]{1,2}月[\u4e00-\u9fa5]{1,2}日', text)
        if cn_date_match:
            ruling_date = cn_date_match.group(0)
        # 然后查找最后出现的日期，通常是裁定日期
        elif dates:
            ruling_date = dates[-1]
        
        # 查找原判决日期
        judgment_date = ""
        judge_match = re.search(r'于(\d{4}年\d{1,2}月\d{1,2}日)作出', text)
        if judge_match:
            judgment_date = judge_match.group(1)
        elif len(dates) > 1:
            judgment_date = dates[0]
        
        return {
            "application_date": application_date,
            "judgment_date": judgment_date,
            "ruling_date": ruling_date
        }
    
    def _extract_court_name(self, text: str) -> str:
        """提取法院名称"""
        court_match = re.search(r'([\u4e00-\u9fa5]+人民法院)', text)
        if court_match:
            return court_match.group(1)
        return ""
    
    def _build_causal_chain(self, legal_info: Dict[str, Any]) -> str:
        """构建因果关系描述"""
        case_info = legal_info.get("case_info", {})
        parties = legal_info.get("parties", [])
        execution = legal_info.get("execution", {})
        timeline = legal_info.get("timeline", {})
        court = legal_info.get("court", "")
        legal_basis = legal_info.get("legal_basis", [])
        
        # 构建因果关系描述
        causal_chain = f"{court}审理了"
        
        # 添加案件类型信息
        if case_info.get("dispute_type"):
            causal_chain += f"{case_info.get('dispute_type')}"
        else:
            causal_chain += f"{case_info.get('case_type', '')}案件"
            
        if case_info.get("case_number"):
            causal_chain += f"（案号：{case_info.get('case_number', '')}）"
        causal_chain += "。"
        
        # 添加当事人信息
        plaintiffs = [p for p in parties if p.get("role") == "原告"]
        defendants = [p for p in parties if p.get("role") == "被告"]
        
        if plaintiffs:
            plaintiff_names = "、".join([p.get("name", "") for p in plaintiffs])
            causal_chain += f"原告{plaintiff_names}"
            
            if execution.get("result", "") and "申请" in execution.get("result", ""):
                causal_chain += f"申请{execution.get('result', '')}，"
        
        if defendants:
            defendant_names = "、".join([p.get("name", "") for p in defendants])
            causal_chain += f"被告{defendant_names}"
            
            if execution.get("reason", ""):
                causal_chain += f"因{execution.get('reason', '')}，"
        
        # 添加法律依据
        if legal_basis:
            laws = "、".join([f"《{law.get('law_name', '')}》第{law.get('article', '')}条" for law in legal_basis])
            causal_chain += f"依据{laws}，"
        
        # 添加执行结果
        if execution.get("result", ""):
            causal_chain += f"法院裁定{execution.get('result', '')}。"
        
        # 添加时间信息
        if timeline.get("ruling_date", ""):
            causal_chain += f"裁定于{timeline.get('ruling_date', '')}生效。"
        
        return causal_chain
    
    def process_legal_document(self, doc: Dict[str, Any]) -> Dict[str, Any]:
        """
        处理整个法律文档
        
        Args:
            doc: 包含文档内容的字典
            
        Returns:
            增加了法律实体和事件的文档
        """
        content = doc.get('content', '')
        if not content:
            return doc
        
        # 提取法律信息
        legal_info = self.extract_legal_info(content)
        
        # 更新文档
        doc['legal_info'] = legal_info
        
        # 提取NLP处理结果
        nlp_result = self.process(content)
        doc['nlp_results'] = nlp_result
        
        return doc
