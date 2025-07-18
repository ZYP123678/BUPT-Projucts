import os
import logging
import gradio as gr
import pandas as pd
import json
import re
from pathlib import Path
from typing import Dict, List, Any, Optional
from datetime import datetime
from PIL import Image
import io
import base64

# 导入法律文档处理相关模块
from src.data_processing.process import LegalDocumentProcessor
from src.data_processing.load import DataLoader

# 导入智谱AI大模型接口
from zhipuai import ZhipuAI

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# 设置智谱AI大模型参数
api_key = "a4ee5eddc13444b19cd81f8d9a20c515.L8KnBoPwCaIX2l0Q"
base_url = "https://open.bigmodel.cn/api/paas/v4/"
model = "glm-4v-flash"

# 论文图片评论反馈存储
PAPER_FEEDBACK = {
    "images": {}  # 以图片名为键存储评论信息
}

LEGAL_FEEDBACK = {
    "documents": {}  # 以文档名为键存储评论信息
}

# 反馈数据保存路径
PAPER_FEEDBACK_PATH = "paper_feedback.json"
LEGAL_FEEDBACK_PATH = "legal_feedback.json"

# 加载已有评论数据
def load_feedback_data():
    """加载已保存的评论反馈数据"""
    global PAPER_FEEDBACK, LEGAL_FEEDBACK
    try:
        if os.path.exists(PAPER_FEEDBACK_PATH):
            with open(PAPER_FEEDBACK_PATH, "r", encoding="utf-8") as f:
                PAPER_FEEDBACK = json.load(f)
        if os.path.exists(LEGAL_FEEDBACK_PATH):
            with open(LEGAL_FEEDBACK_PATH, "r", encoding="utf-8") as f:
                LEGAL_FEEDBACK = json.load(f)
    except Exception as e:
        logger.error(f"加载评论反馈数据失败: {e}")

# 保存评论数据
def save_feedback_data():
    """保存评论反馈数据到文件"""
    try:
        with open(PAPER_FEEDBACK_PATH, "w", encoding="utf-8") as f:
            json.dump(PAPER_FEEDBACK, f, ensure_ascii=False, indent=2)
        with open(LEGAL_FEEDBACK_PATH, "w", encoding="utf-8") as f:
            json.dump(LEGAL_FEEDBACK, f, ensure_ascii=False, indent=2)
        return True
    except Exception as e:
        logger.error(f"保存评论反馈数据失败: {e}")
        return False

# 初始加载评论数据
load_feedback_data()

# 全局评估数据存储
EVALUATION_DATA = {
    "files": {},  # 存储每个文件的评估记录
    "system_score": {
        "average": 0,
        "count": 0,
        "updated_at": ""
    }
}

# 获取图片文件列表
def get_photo_list(photo_dir: str) -> List[str]:
    """获取指定目录下的所有图片文件"""
    try:
        if not os.path.exists(photo_dir):
            logger.warning(f"图片目录不存在: {photo_dir}")
            return []
        
        if not os.path.isdir(photo_dir):
            logger.warning(f"不是有效目录: {photo_dir}")
            return []
        
        photo_list = []
        for file in os.listdir(photo_dir):
            file_path = os.path.join(photo_dir, file)
            if os.path.isfile(file_path) and file.lower().endswith(('.jpg', '.jpeg', '.png')):
                photo_list.append(file)
        
        logger.info(f"在 {photo_dir} 中找到 {len(photo_list)} 张图片")
        return photo_list
    except Exception as e:
        logger.error(f"获取图片列表出错: {e}")
        return []

# 更新图片列表下拉菜单
def update_photo_dropdown(photo_dir: str):
    """更新图片下拉菜单的选项"""
    photos = get_photo_list(photo_dir)
    return gr.update(choices=photos, value=photos[0] if photos else None)

# 加载选定的图片
def load_selected_photo(photo_dir: str, filename: str):
    """加载选定的图片"""
    if not filename:
        return None, "请选择一张图片"
    
    file_path = os.path.join(photo_dir, filename)
    try:
        img = Image.open(file_path)
        return img, f"已加载: {filename}"
    except Exception as e:
        logger.error(f"加载图片出错: {e}")
        return None, f"加载图片失败: {str(e)}"

# 获取目录下的文件列表
def get_file_list(data_dir: str) -> List[str]:
    """获取指定目录下的所有文本文件"""
    try:
        if not os.path.exists(data_dir):
            logger.warning(f"目录不存在: {data_dir}")
            return []
        
        if not os.path.isdir(data_dir):
            logger.warning(f"不是有效目录: {data_dir}")
            return []
        
        file_list = []
        for file in os.listdir(data_dir):
            file_path = os.path.join(data_dir, file)
            if os.path.isfile(file_path) and file.lower().endswith(('.txt', '.json', '.md', '.html', '.pdf')):
                file_list.append(file)
        
        logger.info(f"在 {data_dir} 中找到 {len(file_list)} 个文件")
        return file_list
    except Exception as e:
        logger.error(f"获取文件列表出错: {e}")
        return []

# 更新文件列表下拉菜单
def update_file_dropdown(data_dir: str):
    """更新文件下拉菜单的选项"""
    files = get_file_list(data_dir)
    return gr.update(choices=files, value=files[0] if files else None)

# 使用智谱AI大模型抽取法律文档信息
def extract_with_llm(text: str, filename: str = None) -> Dict[str, Any]:
    """
    使用智谱AI大模型抽取法律文档信息
    
    Args:
        text: 法律文档文本
        filename: 可选的文件名，用于评论记录

    Returns:
        抽取的结构化法律信息
    """
    # 创建ZhipuAI实例
    client = ZhipuAI(
        api_key=api_key,
        base_url=base_url
    )
    
    previous_feedback = []
    if filename and filename in LEGAL_FEEDBACK["documents"]:
        for fb in LEGAL_FEEDBACK["documents"][filename]:
            previous_feedback.append(fb["feedback"])
    
    # 构建提示词，要求返回结构化的JSON格式
    prompt = f"""
    请从以下法律文档中提取关键信息，并以JSON格式返回:
    {text}
    
    请按照以下JSON格式返回提取的信息:
    {{
      "case_info": {{
        "case_number": "案件编号",
        "case_type": "案件类型",
        "dispute_type": "纠纷类型"
      }},
      "parties": [
        {{
          "name": "当事人姓名/名称",
          "role": "角色(原告/被告/代理人等)",
          "type": "类型(个人/组织)",
          "details": {{
            "gender": "性别",
            "birth_date": "出生日期",
            "ethnicity": "民族",
            "occupation": "职业"
          }}
        }}
      ],
      "legal_basis": [
        {{
          "law_name": "法律名称",
          "article": "条款号"
        }}
      ],
      "execution": {{
        "result": "执行结果",
        "measures": ["执行措施1", "执行措施2"],
        "reason": "执行理由",
        "money_amount": "涉案金额"
      }},
      "timeline": {{
        "application_date": "申请日期",
        "judgment_date": "判决日期",
        "ruling_date": "裁定日期"
      }},
      "court": "审理法院",
      "legal_personnel": [
        {{
          "name": "人员姓名",
          "role": "角色(法官/书记员等)"
        }}
      ],
      "causal_chain": "案件摘要描述"
    }}
    
    请确保提取准确的信息，如果某项信息在文档中不存在，对应的值设为空字符串或空数组。
    注意需要保留人物信息的具体细节，不要简化结构。
    """
    
    if previous_feedback:
        feedback_section = "\n\n特别注意以下问题（基于之前的提取反馈）：\n"
        for i, fb in enumerate(previous_feedback):
            feedback_section += f"{i+1}. {fb}\n"
        
        # 在基本提示词后添加反馈部分
        prompt = prompt + feedback_section
        logger.info(f"为文档 {filename} 添加了 {len(previous_feedback)} 条历史反馈")
    else:
        prompt = prompt
    
    try:
        # 调用大模型API
        response = client.chat.completions.create(
            model=model,
            messages=[{"role": "user", "content": prompt}]
        )
        
        # 提取模型返回的文本
        result = response.choices[0].message.content
        
        # 处理返回结果，移除可能存在的markdown代码块标记
        result = result.replace("```json", "").replace("```", "").strip()
        
        # 移除可能存在的注释
        result = re.sub(r'#.*', '', result)
        
        # 解析JSON
        extracted_info = json.loads(result)
        
        return extracted_info
    except Exception as e:
        logger.error(f"大模型抽取失败: {e}")
        return {"error": f"大模型抽取失败: {str(e)}"}
    
# 使用智谱AI大模型抽取图片中的论文信息
def extract_paper_info_from_image(photo_dir: str, filename: str):
    """使用智谱AI大模型从图片中提取论文信息"""
    if not filename:
        return "请先选择一张图片", {}
    
    file_path = os.path.join(photo_dir, filename)
    
    try:
        # 读取图片并转为base64或直接发送文件路径
        with open(file_path, "rb") as image_file:
            # 将图片编码为base64
            encoded_image = base64.b64encode(image_file.read()).decode('utf-8')
            image_url = f"data:image/jpeg;base64,{encoded_image}"
        
        # 创建ZhipuAI实例
        client = ZhipuAI(
            api_key=api_key,
            base_url=base_url
        )
        
        previous_feedback = []
        if filename in PAPER_FEEDBACK["images"]:
            for fb in PAPER_FEEDBACK["images"][filename]:
                previous_feedback.append(fb["feedback"])
        
        
        # 构建提示词，要求大模型分析图片并提取论文信息
        prompt = """请分析这张论文图片，并提取以下信息：
1. 论文标题
2. 作者列表
3. 机构/大学名称
4. 摘要内容
5. 关键词（如果有）

请以JSON格式返回提取的结果，格式如下：
{
  "title": "论文标题",
  "authors": ["作者1", "作者2", ...],
  "affiliations": ["机构1", "机构2", ...],
  "abstract": {：
    "背景和目的": "研究的动机和解决的问题",
    "研究方法": "采用的技术路线和实验设计",
    "研究结果": "主要发现和数据",
    "研究结论": "研究意义和影响"
  },
  "keywords": ["关键词1", "关键词2", ...]
}

如果某项信息无法从图片中提取，请将对应字段设为空字符串或空数组。请尽可能准确地提取信息。"""

        if previous_feedback:
            feedback_section = "\n\n特别注意以下问题（基于之前的提取反馈）：\n"
            for i, fb in enumerate(previous_feedback):
                feedback_section += f"{i+1}. {fb}\n"
            
            # 在基本提示词后添加反馈部分
            prompt = prompt + feedback_section
            logger.info(f"为图片 {filename} 添加了 {len(previous_feedback)} 条历史反馈")
        
        # 调用大模型API
        response = client.chat.completions.create(
            model=model,  # 使用支持图像的模型 glm-4v-flash
            messages=[
                {
                    "role": "user", 
                    "content": [
                        {"type": "text", "text": prompt},
                        {"type": "image_url", "image_url": {"url": image_url}}
                    ]
                }
            ]
        )
        
        # 提取模型返回的文本
        result = response.choices[0].message.content
        
        # 处理返回结果，提取JSON部分
        # 移除可能存在的markdown代码块标记
        result = re.sub(r'```json|```', '', result).strip()
        
        # 尝试解析JSON
        try:
            paper_info = json.loads(result)
            # 生成结果报告
            report = generate_paper_report(paper_info)
            return report, paper_info
        except json.JSONDecodeError as e:
            logger.error(f"解析JSON失败: {e}")
            # 尝试手动提取JSON部分
            json_match = re.search(r'\{.*\}', result, re.DOTALL)
            if json_match:
                try:
                    paper_info = json.loads(json_match.group(0))
                    report = generate_paper_report(paper_info)
                    return report, paper_info
                except:
                    pass
            
            return f"解析结果失败。原始返回：\n\n{result}", {"error": "解析失败", "raw_result": result}
            
    except Exception as e:
        logger.error(f"提取论文信息时出错: {e}")
        return f"提取论文信息失败: {str(e)}", {"error": str(e)}

# 生成论文信息报告
def generate_paper_report(paper_info: dict) -> str:
    """生成论文信息报告"""
    if "error" in paper_info:
        return f"提取错误: {paper_info['error']}"
    
    report = ["## 论文信息抽取结果", ""]
    
    # 添加标题
    title = paper_info.get('title', '')
    if title:
        report.append(f"### 标题")
        report.append(f"{title}")
        report.append("")
    
    # 添加作者信息
    authors = paper_info.get('authors', [])
    if authors:
        report.append("### 作者")
        authors_str = "、".join(authors)
        report.append(f"{authors_str}")
        report.append("")
    
    # 添加机构信息
    affiliations = paper_info.get('affiliations', [])
    if affiliations:
        report.append("### 机构")
        for aff in affiliations:
            report.append(f"- {aff}")
        report.append("")
    
    # 添加摘要
    abstract = paper_info.get('abstract', '')
    if abstract:
        report.append("### 摘要")
        report.append(f"{abstract}")
        report.append("")
    
    # 添加关键词
    keywords = paper_info.get('keywords', [])
    if keywords:
        report.append("### 关键词")
        keywords_str = "、".join(keywords)
        report.append(f"{keywords_str}")
        report.append("")
    
    return "\n".join(report)

def save_paper_feedback_entry(filename, scores, feedback_text):
    """保存图片评分和反馈意见"""
    global PAPER_FEEDBACK
    
    # 计算加权总分
    weights = {
        "title": 25,
        "author": 20,
        "affiliation": 15,
        "abstract": 30,
        "keywords": 10
    }
    weighted_score = calculate_weighted_score(scores, weights)
    
    # 只有当评分较低且有反馈内容时才保存
    if weighted_score < 4.0 and feedback_text.strip():
        if filename not in PAPER_FEEDBACK["images"]:
            PAPER_FEEDBACK["images"][filename] = []
            
        # 添加新反馈
        PAPER_FEEDBACK["images"][filename].append({
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "scores": scores,
            "weighted_score": weighted_score,
            "feedback": feedback_text
        })
        
        # 保存到文件
        save_feedback_data()
        
        return f"""### 评分: {weighted_score}/5
        
> ✅ **已保存您的反馈意见**
>
> 下次提取此图片时，系统将参考您的建议进行优化。"""
    else:
        if not feedback_text.strip():
            note = "未提供具体反馈意见"
        elif weighted_score >= 4.0:
            note = "评分较高，无需保存反馈"
        
        return f"""### 评分: {weighted_score}/5

> ℹ️ **{note}**
>
> 仅保存低评分(<4.0)且有具体反馈的评论。"""

def save_legal_feedback_entry(filename, scores, feedback_text):
    """保存法律文档评分和反馈意见"""
    global LEGAL_FEEDBACK
    
    # 计算加权总分
    weights = {
        "case_info": 15,
        "parties": 25,
        "legal_basis": 20,
        "execution": 15,
        "timeline": 10,
        "personnel": 5,
        "summary": 10
    }
    weighted_score = calculate_weighted_score(scores, weights)
    
    # 只有当评分较低且有反馈内容时才保存
    if weighted_score < 4.0 and feedback_text.strip():
        if filename not in LEGAL_FEEDBACK["documents"]:
            LEGAL_FEEDBACK["documents"][filename] = []
            
        # 添加新反馈
        LEGAL_FEEDBACK["documents"][filename].append({
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "scores": scores,
            "weighted_score": weighted_score,
            "feedback": feedback_text
        })
        
        # 保存到文件
        save_feedback_data()
        
        return f"""### 评分: {weighted_score}/5
        
> ✅ **已保存您的反馈意见**
>
> 下次提取此文档时，系统将参考您的建议进行优化。"""
    else:
        if not feedback_text.strip():
            note = "未提供具体反馈意见"
        elif weighted_score >= 4.0:
            note = "评分较高，无需保存反馈"
        
        return f"""### 评分: {weighted_score}/5

> ℹ️ **{note}**
>
> 仅保存低评分(<4.0)且有具体反馈的评论。"""

# 处理单文件分析
def analyze_legal_document(data_dir: str, filename: str, extraction_method: str):
    """分析单个法律文件 - 提取法律文档关键信息"""
    file_path = os.path.join(data_dir, filename)
    logger.info(f"开始分析法律文件: {file_path}")
    
    # 读取并处理文件
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        logger.error(f"读取文件出错: {e}")
        return (
            f"## 处理出错！\n错误信息：读取文件失败: {str(e)}",
            "", "", {}
        )
    
    # 根据选择的方法进行抽取
    try:
        if extraction_method == "规则+NLP抽取":
            # 使用规则+NLP方法
            processor = LegalDocumentProcessor(batch_size=8)
            legal_info = processor.extract_legal_info(content)
        else:
            # 使用大模型方法
            logger.info("使用智谱AI大模型进行信息抽取")
            legal_info = extract_with_llm(content)
        
        # 生成结果报告
        legal_report = generate_legal_report(legal_info)
        
        return (
            f"## 法律文件抽取完成：{filename} ",
            content, 
            legal_report,
            legal_info
        )
    except Exception as e:
        logger.error(f"处理文件时出错: {e}")
        return (
            f"## 处理出错！\n错误信息：处理文件失败: {str(e)}",
            content, "", {}
        )

def analyze_legal_pdf(data_dir: str, filename: str, extraction_method: str):
    """分析单个PDF法律文件"""
    file_path = os.path.join(data_dir, filename)
    logger.info(f"开始分析PDF法律文件: {file_path}")
    
    # 使用DataLoader读取PDF
    loader = DataLoader(data_dir)
    try:
        doc = loader.read_pdf_file(file_path)
        content = doc.get('content', '')
        
        if not content:
            return (
                f"## 处理出错！\n错误信息：PDF内容为空",
                "", "", {}
            )
        
        # 根据选择的方法进行抽取
        if extraction_method == "规则+NLP抽取":
            # 使用规则+NLP方法
            processor = LegalDocumentProcessor(batch_size=8)
            legal_info = processor.extract_legal_info(content)
        else:
            # 使用大模型方法
            logger.info("使用智谱AI大模型进行信息抽取")
            legal_info = extract_with_llm(content)
        
        # 生成结果报告
        legal_report = generate_legal_report(legal_info)
        
        return (
            f"## 法律文件抽取完成：{filename} ",
            content, 
            legal_report,
            legal_info
        )
    except Exception as e:
        logger.error(f"处理PDF文件时出错: {e}")
        return (
            f"## 处理出错！\n错误信息：处理PDF文件失败: {str(e)}",
            "", "", {}
        )

def generate_legal_report(legal_info: dict) -> str:
    """生成法律信息报告"""
    if "error" in legal_info:
        return f"提取错误: {legal_info['error']}"
    
    report = ["## 法律文书信息抽取结果", ""]
    
    # 添加法院信息
    court = legal_info.get('court', '')
    if court:
        report.append(f"### 法院")
        report.append(f"{court}")
        report.append("")
    
    # 添加案件信息
    case_info = legal_info.get('case_info', {})
    if case_info:
        report.append("### 案件信息")
        if case_info.get('case_number'):
            report.append(f"- **案号**: {case_info.get('case_number')}")
        if case_info.get('case_type'):
            report.append(f"- **案件类型**: {case_info.get('case_type')}")
        if case_info.get('dispute_type'):
            report.append(f"- **纠纷类型**: {case_info.get('dispute_type')}")
        report.append("")
    
    # 添加当事人信息
    parties = legal_info.get('parties', [])
    if parties:
        report.append("### 当事人信息")
        for party in parties:
            name = party.get('name', '')
            role = party.get('role', '')
            party_type = party.get('type', '')
            type_text = f" ({party_type})" if party_type else ""
            report.append(f"- **{role}**: {name}{type_text}")
            
            # 添加详细信息
            details = party.get('details', {})
            if details:
                for k, v in details.items():
                    if v:
                        # 转换键名为中文
                        if k == "gender":
                            k_name = "性别"
                        elif k == "birth_date":
                            k_name = "出生日期"
                        elif k == "ethnicity":
                            k_name = "民族"
                        elif k == "occupation":
                            k_name = "职业"
                        else:
                            k_name = k
                        report.append(f"  - {k_name}: {v}")
        report.append("")
    
    # 添加法律依据
    legal_basis = legal_info.get('legal_basis', [])
    if legal_basis:
        report.append("### 法律依据")
        for law in legal_basis:
            law_name = law.get('law_name', '')
            article = law.get('article', '')
            report.append(f"- 《{law_name}》第{article}条")
        report.append("")
    
    # 添加执行情况
    execution = legal_info.get('execution', {})
    if execution:
        report.append("### 执行情况")
        if execution.get('result'):
            report.append(f"- **结果**: {execution.get('result')}")
        if execution.get('reason'):
            report.append(f"- **原因**: {execution.get('reason')}")
        if execution.get('money_amount'):
            report.append(f"- **涉案金额**: {execution.get('money_amount')}")
        
        # 添加执行措施
        measures = execution.get('measures', [])
        if measures:
            report.append("- **执行措施**:")
            for measure in measures:
                report.append(f"  - {measure}")
        report.append("")
    
    # 添加法律人员
    personnel = legal_info.get('legal_personnel', [])
    if personnel:
        report.append("### 法律人员")
        for person in personnel:
            name = person.get('name', '')
            role = person.get('role', '')
            report.append(f"- **{role}**: {name}")
        report.append("")
    
    # 添加时间线
    timeline = legal_info.get('timeline', {})
    if timeline:
        report.append("### 时间节点")
        if timeline.get('application_date'):
            report.append(f"- **申请日期**: {timeline.get('application_date')}")
        if timeline.get('judgment_date'):
            report.append(f"- **判决日期**: {timeline.get('judgment_date')}")
        if timeline.get('ruling_date'):
            report.append(f"- **裁定日期**: {timeline.get('ruling_date')}")
        report.append("")
    
    # 添加因果关系描述
    causal_chain = legal_info.get('causal_chain', '')
    if causal_chain:
        report.append("### 案件摘要")
        report.append(causal_chain)
    
    return "\n".join(report)

# 计算加权评分
def calculate_weighted_score(scores, weights):
    """计算加权评分"""
    total_score = 0.0
    total_weight = sum(weights.values())
    
    for category, score in scores.items():
        if category in weights:
            total_score += score * weights[category] / total_weight
    
    return round(total_score, 2)

# 构建Gradio界面
def build_interface():
    """构建法律文档与论文信息抽取界面"""
    
    with gr.Blocks(title="法律文档与论文信息抽取系统", css=".gradio-container {max-width: 1100px}") as app:
        gr.Markdown("# ⚖️ 法律文档与论文信息抽取系统")
        gr.Markdown("### 自动提取法律文书与学术论文中的关键信息")
        
        # 全局状态存储
        current_file = gr.State("")
        current_photo = gr.State({})
        current_filename = gr.State("")  # 保存当前图片文件名
        current_extraction_method = gr.State("规则+NLP抽取")  # 保存当前抽取方式
        
        # 主界面 - 使用Tabs分隔不同功能
        with gr.Tabs() as main_tabs:
            # 法律文档Tab - 原有功能
            with gr.TabItem("法律文档处理"):
                with gr.Row():
                    # 左侧控制面板
                    with gr.Column(scale=1):
                        # 数据目录选择
                        data_dir = gr.Textbox(
                            label="数据目录路径",
                            value="C:\\Codefield\\info_know\\3\\Data\\Legal",
                            placeholder="请输入法律文档目录"
                        )
                        
                        # 文件选择
                        file_dropdown = gr.Dropdown(
                            label="选择法律文件",
                            choices=get_file_list("C:\\Codefield\\info_know\\3\\Data\\Legal"),
                            interactive=True
                        )
                        refresh_btn = gr.Button("🔄 刷新文件列表", variant="secondary")
                        
                        # 添加抽取方式选择
                        extraction_method = gr.Radio(
                            label="选择抽取方式",
                            choices=["规则+NLP抽取", "大模型抽取"],
                            value="规则+NLP抽取",
                            info="大模型抽取使用智谱AI GLM-4-flash模型"
                        )
                        
                        # 分析按钮
                        analyze_btn = gr.Button("⚖️ 抽取法律信息", variant="primary", size="lg")
                        
                        # 结果摘要
                        result_summary = gr.Markdown("## 等待抽取...")
                        
                        # 添加评分区域
                        gr.Markdown("### 质量评分")
                        
                        case_info_score = gr.Slider(
                            label="案件信息 (15%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估案号、案件类型等信息提取质量"
                        )
                        parties_score = gr.Slider(
                            label="当事人信息 (25%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估原告/被告识别准确性"
                        )
                        legal_basis_score = gr.Slider(
                            label="法律依据 (20%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估法律条文提取准确性"
                        )
                        execution_score = gr.Slider(
                            label="执行情况 (15%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估执行结果提取准确性"
                        )
                        timeline_score = gr.Slider(
                            label="时间节点 (10%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估时间信息提取准确性"
                        )
                        personnel_score = gr.Slider(
                            label="法律人员 (5%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估法官等人员提取准确性"
                        )
                        summary_score = gr.Slider(
                            label="案件摘要 (10%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估案件摘要生成质量"
                        )
                        
                        # 添加反馈评论框 - 根据抽取方式条件显示
                        with gr.Group(visible=False) as legal_feedback_group:
                            legal_feedback = gr.Textbox(
                                label="反馈意见（低评分时请提供具体问题）",
                                placeholder="例如：案号提取不准确；当事人角色有误；法律依据不完整...",
                                lines=3
                            )
                        
                        # 评分按钮和结果
                        score_btn = gr.Button("计算总分并保存反馈", variant="secondary")
                        score_result = gr.Markdown("### 等待评分...")
                        system_score = gr.Markdown("### 系统整体评分: 暂无数据")
    
                    # 结果显示区域
                    with gr.Column(scale=2):
                        with gr.Tabs() as result_tabs:
                            with gr.Tab("原始内容"):
                                file_content = gr.Textbox(label="文件内容", lines=25, interactive=False)
                            with gr.Tab("法律信息"):
                                legal_result = gr.Markdown()
                            with gr.Tab("反馈评论"):
                                legal_comments = gr.Markdown("### 历史反馈\n\n选择文件并使用大模型抽取后显示历史反馈")
            
            # 论文图片Tab - 新增功能
            with gr.TabItem("论文图片处理"):
                with gr.Row():
                    # 左侧控制面板
                    with gr.Column(scale=1):
                        # 图片目录选择
                        photo_dir = gr.Textbox(
                            label="图片目录路径",
                            value="C:\\Codefield\\info_know\\3\\Data\\Photo",
                            placeholder="请输入论文图片目录"
                        )
                        
                        # 图片选择
                        photo_dropdown = gr.Dropdown(
                            label="选择论文图片",
                            choices=get_photo_list("C:\\Codefield\\info_know\\3\\Data\\Photo"),
                            interactive=True
                        )
                        refresh_photo_btn = gr.Button("🔄 刷新图片列表", variant="secondary")
                        
                        # 提取信息按钮
                        extract_photo_btn = gr.Button("🔍 提取论文信息", variant="primary", size="lg")
                        
                        # 状态显示
                        photo_status = gr.Markdown("## 等待选择图片...")
                        
                        # 论文质量评分
                        gr.Markdown("### 提取质量评分")
                        
                        title_score = gr.Slider(
                            label="标题提取 (25%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估标题提取准确性"
                        )
                        author_score = gr.Slider(
                            label="作者提取 (20%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估作者信息提取准确性"
                        )
                        affiliation_score = gr.Slider(
                            label="机构提取 (15%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估机构信息提取准确性"
                        )
                        abstract_score = gr.Slider(
                            label="摘要提取 (30%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估摘要内容提取准确性"
                        )
                        keywords_score = gr.Slider(
                            label="关键词提取 (10%)", 
                            minimum=0, maximum=5, step=0.5, value=3,
                            info="评估关键词提取准确性"
                        )
                        
                        # 添加评论框
                        paper_feedback = gr.Textbox(
                            label="反馈意见（低评分时请提供具体问题）",
                            placeholder="例如：标题提取不完整，漏掉了副标题；作者顺序有误；摘要缺少研究方法部分...",
                            lines=3
                        )
                        
                        # 评分按钮和结果
                        photo_score_btn = gr.Button("计算总分并保存反馈", variant="secondary")
                        photo_score_result = gr.Markdown("### 等待评分...")
                    
                    # 右侧图片和结果显示区域
                    with gr.Column(scale=2):
                        with gr.Tabs() as photo_result_tabs:
                            with gr.Tab("图片预览"):
                                photo_preview = gr.Image(type="pil", label="论文图片")
                            with gr.Tab("提取结果"):
                                paper_result = gr.Markdown("等待提取...")
                            with gr.Tab("反馈历史"):
                                paper_comments = gr.Markdown("### 历史反馈\n\n选择图片后显示历史反馈")
        
        # ===== 法律文档部分事件处理 =====
        
        # 目录变化时更新文件列表
        data_dir.change(
            update_file_dropdown,
            inputs=data_dir,
            outputs=file_dropdown
        )
        
        # 刷新文件列表按钮
        refresh_btn.click(
            update_file_dropdown,
            inputs=data_dir,
            outputs=file_dropdown
        )
        
        # 抽取方式变化时更新界面
        def update_extraction_method(method):
            """更新抽取方式并显示/隐藏评论框"""
            show_feedback = (method == "大模型抽取")
            return method, gr.update(visible=show_feedback)
            
        extraction_method.change(
            update_extraction_method,
            inputs=extraction_method,
            outputs=[current_extraction_method, legal_feedback_group]
        )
        
        # 文件选择变化时显示历史反馈
        def show_legal_feedback_history(filename):
            """显示选定文件的历史反馈"""
            if not filename:
                return "### 历史反馈\n\n请先选择一个文件"
                
            if filename in LEGAL_FEEDBACK["documents"]:
                feedback_list = LEGAL_FEEDBACK["documents"][filename]
                if not feedback_list:
                    return "### 历史反馈\n\n该文件暂无反馈记录"
                    
                history = ["### 历史反馈记录", ""]
                for i, fb in enumerate(feedback_list):
                    history.append(f"**反馈 {i+1}** ({fb['timestamp']})")
                    history.append(f"- 评分: {fb['weighted_score']}/5")
                    history.append(f"- 内容: {fb['feedback']}")
                    history.append("")
                    
                return "\n".join(history)
            else:
                return "### 历史反馈\n\n该文件暂无反馈记录"
                
        file_dropdown.change(
            show_legal_feedback_history,
            inputs=file_dropdown,
            outputs=legal_comments
        )
        
        # 分析按钮事件处理函数
        def process_file(data_dir, filename, extraction_method):
            if not filename:
                return "请先选择一个文件", "", "", ""
                
            # 根据文件类型选择不同的处理方法
            if filename.lower().endswith('.pdf'):
                result = analyze_legal_pdf(data_dir, filename, extraction_method)
            else:
                result = analyze_legal_document(data_dir, filename, extraction_method)
            
            # 更新历史反馈显示
            feedback_history = show_legal_feedback_history(filename)
            
            # 更新当前文件状态并返回结果
            return result[0], result[1], result[2], filename, feedback_history
        
        # 绑定分析按钮事件
        analyze_btn.click(
            process_file,
            inputs=[data_dir, file_dropdown, extraction_method],
            outputs=[result_summary, file_content, legal_result, current_file, legal_comments]
        )
        
        # 评分按钮事件处理函数
        def on_calculate_score(
            filename, 
            extraction_method,
            case_info_score, 
            parties_score, 
            legal_basis_score, 
            execution_score, 
            timeline_score, 
            personnel_score, 
            summary_score,
            feedback_text
        ):
            if not filename:
                return "### 请先选择并抽取文件信息", "### 系统整体评分: 暂无数据"
            
            # 定义权重
            weights = {
                "case_info": 15,
                "parties": 25,
                "legal_basis": 20,
                "execution": 15,
                "timeline": 10,
                "personnel": 5,
                "summary": 10
            }
            
            # 收集评分
            scores = {
                "case_info": case_info_score,
                "parties": parties_score,
                "legal_basis": legal_basis_score,
                "execution": execution_score,
                "timeline": timeline_score,
                "personnel": personnel_score,
                "summary": summary_score
            }
            
            # 计算加权评分
            weighted_score = calculate_weighted_score(scores, weights)
            
            # 更新全局评估数据
            global EVALUATION_DATA
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            if filename not in EVALUATION_DATA["files"]:
                EVALUATION_DATA["files"][filename] = []
            
            EVALUATION_DATA["files"][filename].append({
                "timestamp": timestamp,
                "scores": scores,
                "weighted_score": weighted_score
            })
            
            # 计算系统整体评分
            all_files_scores = []
            for file_evals in EVALUATION_DATA["files"].values():
                if file_evals:
                    # 只取每个文件的最新评分
                    all_files_scores.append(file_evals[-1]["weighted_score"])
            
            if all_files_scores:
                system_avg_score = round(sum(all_files_scores) / len(all_files_scores), 2)
                EVALUATION_DATA["system_score"] = {
                    "average": system_avg_score,
                    "count": len(all_files_scores),
                    "updated_at": timestamp
                }
            
            # 如果是大模型抽取，且有反馈意见，则保存反馈
            if extraction_method == "大模型抽取" and feedback_text.strip():
                feedback_result = save_legal_feedback_entry(filename, scores, feedback_text)
                file_score_result = feedback_result
                # 更新历史反馈显示
                updated_feedback = show_legal_feedback_history(filename)
                
                system_score_result = f"### 系统整体评分: {EVALUATION_DATA['system_score'].get('average', 0)}/5 (基于{EVALUATION_DATA['system_score'].get('count', 0)}个文件)"
                
                return file_score_result, system_score_result, updated_feedback
            else:
                # 返回评分结果
                file_score_result = f"### 当前文件评分: {weighted_score}/5"
                system_score_result = f"### 系统整体评分: {EVALUATION_DATA['system_score'].get('average', 0)}/5 (基于{EVALUATION_DATA['system_score'].get('count', 0)}个文件)"
                
                return file_score_result, system_score_result, legal_comments.value
        
        # 绑定评分按钮事件
        score_btn.click(
            on_calculate_score,
            inputs=[
                current_file,
                current_extraction_method,
                case_info_score, 
                parties_score, 
                legal_basis_score, 
                execution_score, 
                timeline_score, 
                personnel_score, 
                summary_score,
                legal_feedback
            ],
            outputs=[score_result, system_score, legal_comments]
        )
        
        # ===== 论文图片部分事件处理 =====
        
        # 目录变化时更新图片列表
        photo_dir.change(
            update_photo_dropdown,
            inputs=photo_dir,
            outputs=photo_dropdown
        )
        
        # 刷新图片列表按钮
        refresh_photo_btn.click(
            update_photo_dropdown,
            inputs=photo_dir,
            outputs=photo_dropdown
        )
        
        # 显示图片历史反馈
        def show_paper_feedback_history(filename):
            """显示选定图片的历史反馈"""
            if not filename:
                return "### 历史反馈\n\n请先选择一张图片", ""
                
            if filename in PAPER_FEEDBACK["images"]:
                feedback_list = PAPER_FEEDBACK["images"][filename]
                if not feedback_list:
                    return "### 历史反馈\n\n该图片暂无反馈记录", filename
                    
                history = ["### 历史反馈记录", ""]
                for i, fb in enumerate(feedback_list):
                    history.append(f"**反馈 {i+1}** ({fb['timestamp']})")
                    history.append(f"- 评分: {fb['weighted_score']}/5")
                    history.append(f"- 内容: {fb['feedback']}")
                    history.append("")
                    
                return "\n".join(history), filename
            else:
                return "### 历史反馈\n\n该图片暂无反馈记录", filename
        
        # 加载图片事件
        def load_selected_photo(photo_dir, filename):
            """加载选定的图片"""
            if not filename:
                return None, "请选择一张图片", "### 历史反馈\n\n请先选择一张图片", ""
            
            file_path = os.path.join(photo_dir, filename)
            try:
                img = Image.open(file_path)
                # 获取历史反馈
                feedback_history, _ = show_paper_feedback_history(filename)
                return img, f"已加载: {filename}", feedback_history, filename
            except Exception as e:
                logger.error(f"加载图片出错: {e}")
                return None, f"加载图片失败: {str(e)}", "### 加载图片失败", ""
        
        # 自动加载选中的图片
        photo_dropdown.change(
            load_selected_photo,
            inputs=[photo_dir, photo_dropdown],
            outputs=[photo_preview, photo_status, paper_comments, current_filename]
        )
        
        # 提取论文信息事件处理函数
        def process_photo(photo_dir, filename):
            """处理图片并提取论文信息"""
            if not filename:
                return "请先选择一张图片", {}
                
            # 加载图片预览
            try:
                # 提取论文信息
                result, paper_info = extract_paper_info_from_image(photo_dir, filename)
                return result, paper_info
            except Exception as e:
                logger.error(f"处理图片时出错: {e}")
                return f"处理图片时出错: {str(e)}", {}
        
        # 绑定提取信息按钮事件
        extract_photo_btn.click(
            process_photo,
            inputs=[photo_dir, photo_dropdown],
            outputs=[paper_result, current_photo]
        )
        
        # 论文图片评分事件处理函数
        def on_calculate_photo_score(
            paper_info,
            current_filename,
            title_score,
            author_score,
            affiliation_score,
            abstract_score,
            keywords_score,
            feedback_text
        ):
            if not paper_info or (isinstance(paper_info, dict) and not paper_info):
                return "### 请先提取图片信息"
                
            if not current_filename:
                return "### 请先选择一张图片"
            
            # 定义权重
            weights = {
                "title": 25,
                "author": 20,
                "affiliation": 15,
                "abstract": 30,
                "keywords": 10
            }
            
            # 收集评分
            scores = {
                "title": title_score,
                "author": author_score,
                "affiliation": affiliation_score,
                "abstract": abstract_score,
                "keywords": keywords_score
            }
            
            # 保存评论和评分
            feedback_result = save_paper_feedback_entry(current_filename, scores, feedback_text)
            
            # 更新历史反馈显示
            updated_feedback, _ = show_paper_feedback_history(current_filename)
            
            return feedback_result, updated_feedback
        
        # 绑定论文评分按钮事件
        photo_score_btn.click(
            on_calculate_photo_score,
            inputs=[
                current_photo,
                current_filename,
                title_score,
                author_score,
                affiliation_score,
                abstract_score,
                keywords_score,
                paper_feedback
            ],
            outputs=[photo_score_result, paper_comments]
        )
    
    return app
# 初始化智谱AI大模型
def initialize_llm():
    """初始化智谱AI大模型，测试连接是否正常"""
    try:
        client = ZhipuAI(api_key=api_key, base_url=base_url)
        response = client.chat.completions.create(
            model=model,
            messages=[{"role": "user", "content": "你好，请输出'大模型连接成功'字样"}]
        )
        result = response.choices[0].message.content
        if "大模型连接成功" in result:
            logger.info("智谱AI大模型连接正常")
        else:
            logger.warning(f"智谱AI大模型连接测试返回异常: {result}")
    except Exception as e:
        logger.error(f"初始化智谱AI大模型时出错: {e}")

if __name__ == "__main__":
    # 初始化智谱AI大模型
    initialize_llm()
    
    # 启动Gradio应用
    app = build_interface()
    # 使用不同端口避免冲突
    app.launch(share=False, server_port=7861)